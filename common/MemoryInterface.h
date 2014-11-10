#ifndef __MEMORY_INTERFACE_H__
#define __MEMORY_INTERFACE_H__

/* An interface that provides all the memory APIs needed by a BaseDatapath
 * object. Any functions specfic to a memory structure should be declared and
 * defined in a child class, and a child class of BaseDatapath should construct
 * those object types directly instead of this base type.
 */
class MemoryInterface {

  public:
    /* A memory structure is usually divided into multiple smaller blocks, which
     * may be organized as a hierarchy, partitions, etc. This function returns
     * the names/identifiers of these blocks.
     *
     * Args:
     *   names: A reference to a preallocated vector of strings.
     */
    virtual void getMemoryBlocks(std::vector<std::string>& names) = 0;

    /* Returns the names/identifiers of any dedicated registers for specific use
     * cases, if they exist, in the provided @names parameter.
     *
     * Args:
     *   names: A reference to a preallocated vector of strings.
     */
    virtual void getRegisterBlocks(std::vector<std::string>& names) = 0;

    /* Returns the average power consumed by the memory structure.
     */
    virtual void getAveragePower(unsigned int cycles, float &avg_power,
                                float &avg_dynamic, float &avg_leak) = 0;

    /* Returns the total area of the memory structure.
     */
    virtual float getTotalArea() = 0;

    /* Returns the power required for a read operation on the specified memory
     * block.
     */
    /*virtual float getReadPower(std::string block_name) = 0;*/

    /* Returns the energy required for a read operation on the specified memory
     * block.
     */
    virtual float getReadEnergy(std::string block_name) = 0;

    /* Returns the power required for a write operation on the specified memory
     * block.
     */
    /*virtual float getWritePower(std::string block_name) = 0;*/

    /* Returns the energy required for a write operation on the specified memory
     * block.
     */
    virtual float getWriteEnergy(std::string block_name) = 0;

    /* Returns the leakage power consumed when the specified memory block is
     * active.
     */
    virtual float getLeakagePower(std::string block_name) = 0;

    /* Returns the area occupied by the specified memory block.
     */
    virtual float getArea(std::string block_name) = 0;
};

#endif
