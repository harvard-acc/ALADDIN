#ifndef __DATABASE_DEPS_H__
#define __DATABASE_DEPS_H__

// All MySQL database headers needed. Any file needing database support only
// needs to include this header.

#ifdef USE_DB
#include "mysql_connection.h"
#include "mysql_driver.h"
#include "cppconn/driver.h"
#include "cppconn/exception.h"
#include "cppconn/resultset.h"
#include "cppconn/statement.h"
#include "DatabaseConfig.h"
#endif

#endif
