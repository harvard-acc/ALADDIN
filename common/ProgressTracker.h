#ifndef _PROGRESS_STATE_H_
#define _PROGRESS_STATE_H_

#include <stdio.h>
#include <sys/time.h>

/* A class to track progress about a long running task.
 *
 * This can be very useful for knowing how far along trace parsing or some
 * other graph optimization has proceeded, or if a task is stuck somewhere.
 *
 * Progress is recorded and output at intervals of epochs. An epoch of length
 * 0.05 means it spans 5% of the total work that must be done.
 *
 * In addition, additional statistics can be tracked and printed at the end of
 * an epoch as a rate (unit/sec).
 *
 * To avoid problems with buffered outputs, the progress is written into a
 * separate local file and flushed on every update.
 */
class ProgressTracker {
 public:
  /* Construct a new ProgressTracker object.
   *
   * Args:
   *  filename: The name of the file to write output to. If it exists, it will
   *    be overwritten.
   *  _curr_total: A pointer to an external counter indicating the current level
   *    of progress.
   *  _total: The count that _curr_total must reach for this task to be complete.
   *  _epoch_length: The length of an epoch in percent (5% = 0.05).
   */
  ProgressTracker(std::string filename,
                long* _curr_total,
                const long _total,
                const float _epoch_length,
                bool overwrite=true)
      : curr_total(_curr_total), epoch_length(_epoch_length), total(_total) {
    if (overwrite)
      file = fopen(filename.c_str(), "w");
    else
      file = fopen(filename.c_str(), "a");
  }

  ~ProgressTracker() { fclose(file); }

  // Start a new epoch.
  void start_epoch() { start_timing(); }

  /* End the current epoch.
   *
   * By default, this will print a progress update, unless the @print parameter
   * is set to false.
   */
  void end_epoch(bool print = true) {
    end_timing();
    update_progress();
    if (print)
      print_update();
    advance_stats_epoch();
  }

  // End the current epoch and start a new one.
  void start_new_epoch(bool print = true) {
    end_epoch(print);
    start_epoch();
  }

  // Returns true if the current progress has reached the end of an epoch.
  bool at_epoch_end() {
    float pct_prog = ((float)*curr_total) / total;
    return (pct_prog - last_pct_progress >= epoch_length);
  }

  // Add a new stat to track.
  void add_stat(std::string name, long* stat_ptr) {
    stats[name] = { stat_ptr, 0 };
  }

  /* Advance the stats tracker in preparation for a new epoch.
   *
   * This means that the last epoch marker for each stat is set to the current
   * value.
   */
  void advance_stats_epoch() {
    for (auto& pair : stats) {
      pair.second.last_update = *pair.second.current;
    }
  }

  // "Latch" the current progress as the marker for this epoch.
  void update_progress() {
    last_pct_progress = ((float)*curr_total) / total;
  }

  // Timing utility methods.
  void start_timing() { gettimeofday(&start, NULL); }
  void end_timing() { gettimeofday(&end, NULL); }
  float compute_elapsed_time() {
    return (end.tv_sec - start.tv_sec) +
           ((float)(end.tv_usec - start.tv_usec)) / 1e6;
  }

  void print_update() {
    float seconds = compute_elapsed_time();

    fprintf(file, "  %2.0f%%\t(%1.2f seconds elapsed", last_pct_progress * 100,
            seconds);

    for (auto& pair : stats) {
      const std::string& stat_name = pair.first;
      const stat_entry_t& stat = pair.second;
      float rate = ((float)*stat.current - stat.last_update) / seconds;
      fprintf(file, ", %2.0f %s/sec", rate, stat_name.c_str());
    }
    fprintf(file, ")\n");
    fflush(file);
  }

 private:
  struct stat_entry_t {
    long* current;
    long last_update;
  };
  std::map<std::string, stat_entry_t> stats;

  long* curr_total;
  float last_pct_progress;
  struct timeval start;
  struct timeval end;

  FILE* file;

  const float epoch_length;
  const long total;
};

#endif
