#include "qgsodbcresultsetmetadata.h"
#include <QString>

using namespace NS_ODBC;

QgsOdbcResultSetMetadata::QgsOdbcResultSetMetadata( ResultSetMetaDataUnicodeRef &&metadata )
  : mMetadata( std::move(metadata) )
{
}

QString QgsOdbcResultSetMetadata::getCatalogName( unsigned short columnIndex )
{
  return QString::fromStdU16String( mMetadata->getCatalogName( columnIndex ) );
}

QString QgsOdbcResultSetMetadata::getSchemaName( unsigned short columnIndex )
{
  return QString::fromStdU16String( mMetadata->getSchemaName( columnIndex ) );
}

QString QgsOdbcResultSetMetadata::getTableName( unsigned short columnIndex )
{
  return QString::fromStdU16String( mMetadata->getTableName( columnIndex ) );
}

QString QgsOdbcResultSetMetadata::getBaseTableName( unsigned short columnIndex )
{
  return QString::fromStdU16String( mMetadata->getBaseTableName( columnIndex ) );
}

QString QgsOdbcResultSetMetadata::getBaseColumnName( unsigned short columnIndex )
{
  return QString::fromStdU16String( mMetadata->getBaseColumnName( columnIndex ) );
}

unsigned short QgsOdbcResultSetMetadata::getColumnCount()
{
  return  mMetadata->getColumnCount();
}

short QgsOdbcResultSetMetadata::getColumnType( unsigned short columnIndex )
{
  return  mMetadata->getColumnType( columnIndex );
}

std::size_t QgsOdbcResultSetMetadata::getColumnLength( unsigned short columnIndex )
{
  return  mMetadata->getColumnLength( columnIndex );
}

std::size_t QgsOdbcResultSetMetadata::getColumnOctetLength( unsigned short columnIndex )
{
  return  mMetadata->getColumnOctetLength( columnIndex );
}

std::size_t QgsOdbcResultSetMetadata::getColumnDisplaySize( unsigned short columnIndex )
{
  return  mMetadata->getColumnDisplaySize( columnIndex );
}

QString QgsOdbcResultSetMetadata::getColumnLabel( unsigned short columnIndex )
{
  return QString::fromStdU16String( mMetadata->getColumnLabel( columnIndex ) );
}

QString QgsOdbcResultSetMetadata::getColumnName( unsigned short columnIndex )
{
  return QString::fromStdU16String( mMetadata->getColumnName( columnIndex ) );
}

QString QgsOdbcResultSetMetadata::getColumnTypeName( unsigned short columnIndex )
{
  return QString::fromStdU16String( mMetadata->getColumnTypeName( columnIndex ) );
}

unsigned short QgsOdbcResultSetMetadata::getPrecision( unsigned short columnIndex )
{
  return mMetadata->getPrecision( columnIndex );
}

unsigned short QgsOdbcResultSetMetadata::getScale( unsigned short columnIndex )
{
  return mMetadata->getScale( columnIndex );
}

bool QgsOdbcResultSetMetadata::isAutoIncrement( unsigned short columnIndex )
{
  return mMetadata->isAutoIncrement( columnIndex );
}

bool QgsOdbcResultSetMetadata::isCaseSensitive( unsigned short columnIndex )
{
  return mMetadata->isCaseSensitive( columnIndex );
}

bool QgsOdbcResultSetMetadata::isNamed( unsigned short columnIndex )
{
  return mMetadata->isNamed( columnIndex );
}

bool QgsOdbcResultSetMetadata::isNullable( unsigned short columnIndex )
{
  return mMetadata->isNullable( columnIndex );
}

bool QgsOdbcResultSetMetadata::isReadOnly( unsigned short columnIndex )
{
  return mMetadata->isReadOnly( columnIndex );
}

bool QgsOdbcResultSetMetadata::isSearchable( unsigned short columnIndex )
{
  return mMetadata->isSearchable( columnIndex );
}

bool QgsOdbcResultSetMetadata::isSigned( unsigned short columnIndex )
{
  return mMetadata->isSigned( columnIndex );
}
