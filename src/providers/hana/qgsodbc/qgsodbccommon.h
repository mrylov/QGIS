/***************************************************************************
   qgsodbccommon.h
   --------------------------------------
   Date      : 05-07-2023
   Copyright : (C) SAP SE
   Author    : Maxim Rylov
 ***************************************************************************/

/***************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 ***************************************************************************/
#ifndef QGSODBCCOMMON_H
#define QGSODBCCOMMON_H

#include "odbc/Exception.h"
#include "odbc/Types.h"

template<class T>
using QgsOdbcNullable = NS_ODBC::Nullable<T>;

using QgsOdbcException = NS_ODBC::Exception;

using QgsOdbcIndexType = NS_ODBC::IndexType;
using QgsOdbcStatisticsAccuracy = NS_ODBC::StatisticsAccuracy;
using QgsOdbcDataTypes = NS_ODBC::SQLDataTypes;

#endif // QGSODBCCOMMON_H
