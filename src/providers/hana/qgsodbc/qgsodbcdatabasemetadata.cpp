#include "qgsodbcdatabasemetadata.h"
#include "qgsodbcresultset.h"
#include "odbc/DatabaseMetaDataUnicode.h"
#include "odbc/ResultSet.h"
#include <QString>

using namespace NS_ODBC;

namespace
{
  const char16_t *toUtf16( const QString &str )
  {
    return str.isNull() ? nullptr : reinterpret_cast<const char16_t *>( str.utf16() );
  }
}

QgsOdbcDatabaseMetadata::QgsOdbcDatabaseMetadata( DatabaseMetaDataUnicodeRef &&metadata )
  : mMetadata( std::move( metadata ) )
{
}

QgsOdbcResultSet QgsOdbcDatabaseMetadata::getColumns(
  const QString &catalogName,
  const QString &schemaName,
  const QString &tableName,
  const QString &columnName )
{
  return QgsOdbcResultSet( mMetadata->getColumns( toUtf16( catalogName ), toUtf16( schemaName ), toUtf16( tableName ), toUtf16( columnName ) ) );
}

QString QgsOdbcDatabaseMetadata::getDBMSVersion()
{
  std::u16string str = mMetadata->getDBMSVersion();
  return QString::fromUtf16( str.c_str(), str.length() );
}

QgsOdbcResultSet QgsOdbcDatabaseMetadata::getPrimaryKeys(
  const QString &catalogName,
  const QString &schemaName,
  const QString &tableName )
{
  return QgsOdbcResultSet( mMetadata->getPrimaryKeys( toUtf16( catalogName ), toUtf16( schemaName ), toUtf16( tableName ) ) );
}

QgsOdbcResultSet QgsOdbcDatabaseMetadata::getStatistics(
  const QString &catalogName,
  const QString &schemaName,
  const QString &tableName,
  QgsOdbcIndexType indexType,
  QgsOdbcStatisticsAccuracy accuracy )
{
  return QgsOdbcResultSet( mMetadata->getStatistics( toUtf16( catalogName ), toUtf16( schemaName ), toUtf16( tableName ), indexType, accuracy ) );
}
