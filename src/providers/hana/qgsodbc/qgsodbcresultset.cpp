#include "qgsgeometry.h"
#include "qgslogger.h"
#include "qgsodbcresultset.h"
#include "qgsodbcresultsetmetadata.h"
#include <QDate>
#include <QDateTime>
#include <QString>
#include <QTime>
#include <QVariant>

using namespace NS_ODBC;

QgsOdbcResultSet::QgsOdbcResultSet( ResultSetRef &&resultSet )
  : mResultSet( std::move( resultSet ) )
  , mMetadata( mResultSet->getMetaDataUnicode() )
{
}

QgsOdbcResultSet::QgsOdbcResultSet( QgsOdbcResultSet &&resultSet )
  : mResultSet( std::move( resultSet.mResultSet ) )
  , mMetadata( mResultSet->getMetaDataUnicode() )
{}

void QgsOdbcResultSet::close()
{
  mResultSet->close();
}

bool QgsOdbcResultSet::next()
{
  return mResultSet->next();
}

QgsOdbcNullable<bool> QgsOdbcResultSet::getBoolean( unsigned short columnIndex )
{
  return mResultSet->getBoolean( columnIndex );
}

QgsOdbcNullable<std::int8_t> QgsOdbcResultSet::getByte( unsigned short columnIndex )
{
  return mResultSet->getByte( columnIndex );
}

QgsOdbcNullable<std::uint8_t> QgsOdbcResultSet::getUByte( unsigned short columnIndex )
{
  return mResultSet->getUByte( columnIndex );
}

QgsOdbcNullable<std::int16_t> QgsOdbcResultSet::getShort( unsigned short columnIndex )
{
  return mResultSet->getShort( columnIndex );
}

QgsOdbcNullable<std::uint16_t> QgsOdbcResultSet::getUShort( unsigned short columnIndex )
{
  return mResultSet->getUShort( columnIndex );
}

QgsOdbcNullable<std::int32_t> QgsOdbcResultSet::getInt( unsigned short columnIndex )
{
  return mResultSet->getInt( columnIndex );
}

QgsOdbcNullable<std::uint32_t> QgsOdbcResultSet::getUInt( unsigned short columnIndex )
{
  return mResultSet->getUInt( columnIndex );
}

QgsOdbcNullable<std::int64_t> QgsOdbcResultSet::getLong( unsigned short columnIndex )
{
  return mResultSet->getLong( columnIndex );
}

QgsOdbcNullable<std::uint64_t> QgsOdbcResultSet::getULong( unsigned short columnIndex )
{
  return mResultSet->getULong( columnIndex );
}

QgsOdbcNullable<double> QgsOdbcResultSet::getDouble( unsigned short columnIndex )
{
  return mResultSet->getDouble( columnIndex );
}

QgsOdbcNullable<float> QgsOdbcResultSet::getFloat( unsigned short columnIndex )
{
  return mResultSet->getFloat( columnIndex );
}

QDate QgsOdbcResultSet::getDate( unsigned short columnIndex )
{
  Date value = mResultSet->getDate( columnIndex );
  return value.isNull() ?  QDate() : QDate( value->year(), value->month(), value->day() );
}

QDateTime QgsOdbcResultSet::getDateTime( unsigned short columnIndex )
{
  Timestamp value = mResultSet->getTimestamp( columnIndex );
  return value.isNull() ? QDateTime() : QDateTime( QDate( value->year(), value->month(), value->day() ),
         QTime( value->hour(), value->minute(), value->second(), value->milliseconds() ) );
}

QTime QgsOdbcResultSet::getTime( unsigned short columnIndex )
{
  Time value = mResultSet->getTime( columnIndex );
  return value.isNull() ? QTime() : QTime( value->hour(), value->minute(), value->second(), 0 );
}

QString QgsOdbcResultSet::getString( unsigned short columnIndex )
{
  NString str = mResultSet->getNString( columnIndex ) ;
  return str.isNull() ? QString() : QString::fromStdU16String( *str );
}

QVariant QgsOdbcResultSet::getVariant( unsigned short columnIndex )
{
  switch ( mMetadata->getColumnType( columnIndex ) )
  {
    case SQLDataTypes::Bit:
    case SQLDataTypes::Boolean:
    {
      Boolean value = mResultSet->getBoolean( columnIndex );
      return value.isNull() ? QVariant( QVariant::Bool ) : QVariant( *value );
    }
    case SQLDataTypes::Char:
    {
      String str = mResultSet->getString( columnIndex );
      if ( mMetadata->getColumnLength( columnIndex ) == 1 )
      {
        if ( str.isNull() || str->empty() )
          return QVariant( QVariant::Char );
        else
          return QVariant( QChar( str->at( 0 ) ) );
      }
      else
        return str.isNull() ? QVariant( QVariant::String ) : QString::fromUtf8( str->c_str() );
    }
    case SQLDataTypes::WChar:
    {
      NString str = mResultSet->getNString( columnIndex );
      if ( mMetadata->getColumnLength( columnIndex ) == 1 )
      {
        if ( str.isNull() || str->empty() )
          return QVariant( QVariant::Char );
        else
          return QVariant( QChar( str->at( 0 ) ) );
      }
      else
        return str.isNull() ? QVariant( QVariant::String ) : QString::fromStdU16String( str->c_str() );
    }
    case SQLDataTypes::TinyInt:
      if ( mMetadata ->isSigned( columnIndex ) )
      {
        Byte value =  mResultSet->getByte( columnIndex ) ;
        return value.isNull() ? QVariant( QVariant::Int ) : QVariant( static_cast<int>( *value ) );
      }
      else
      {
        UByte value =  mResultSet->getUByte( columnIndex ) ;
        return value.isNull() ? QVariant( QVariant::UInt ) : QVariant( static_cast<uint>( *value ) );
      }
    case SQLDataTypes::SmallInt:
      if ( mMetadata ->isSigned( columnIndex ) )
      {
        Short value =  mResultSet->getShort( columnIndex ) ;
        return value.isNull() ? QVariant( QVariant::Int ) : QVariant( static_cast<int>( *value ) );
      }
      else
      {
        UShort value =  mResultSet->getUShort( columnIndex ) ;
        return value.isNull() ? QVariant( QVariant::UInt ) : QVariant( static_cast<uint>( *value ) );
      }
    case SQLDataTypes::Integer:
      if ( mMetadata ->isSigned( columnIndex ) )
      {
        Int value =  mResultSet->getInt( columnIndex ) ;
        return value.isNull() ? QVariant( QVariant::Int ) : QVariant( static_cast<int>( *value ) );
      }
      else
      {
        UInt value =  mResultSet->getUInt( columnIndex ) ;
        return value.isNull() ? QVariant( QVariant::UInt ) : QVariant( static_cast<uint>( *value ) );
      }
    case SQLDataTypes::BigInt:
      if ( mMetadata ->isSigned( columnIndex ) )
      {
        Long value =  mResultSet->getLong( columnIndex ) ;
        return value.isNull() ? QVariant( QVariant::LongLong ) : QVariant( static_cast<qlonglong>( *value ) );
      }
      else
      {
        ULong value =  mResultSet->getULong( columnIndex ) ;
        return value.isNull() ? QVariant( QVariant::ULongLong ) : QVariant( static_cast<qulonglong>( *value ) );
      }
    case SQLDataTypes::Real:
    {
      Float value = mResultSet->getFloat( columnIndex );
      return value.isNull() ?  QVariant( QVariant::Double ) : QVariant( static_cast<double>( *value ) ) ;
    }
    case SQLDataTypes::Double:
    case SQLDataTypes::Decimal:
    case SQLDataTypes::Float:
    case SQLDataTypes::Numeric:
    {
      Double value = mResultSet->getDouble( columnIndex );
      return value.isNull() ?  QVariant( QVariant::Double ) : QVariant( static_cast<double>( *value ) ) ;
    }
    case SQLDataTypes::Date:
    case SQLDataTypes::TypeDate:
    {
      Date value = mResultSet->getDate( columnIndex );
      return value.isNull() ? QVariant( QVariant::Date ) : QVariant( QDate( value->year(), value->month(), value->day() ) );
    }
    case SQLDataTypes::Time:
    case SQLDataTypes::TypeTime:
    {
      Time value = mResultSet->getTime( columnIndex );
      return value.isNull() ? QVariant( QVariant::Time ) : QVariant( QTime( value->hour(), value->minute(), value->second(), 0 ) );
    }
    case SQLDataTypes::Timestamp:
    case SQLDataTypes::TypeTimestamp:
    {
      Timestamp value = mResultSet->getTimestamp( columnIndex );
      return value.isNull() ? QVariant( QVariant::DateTime ) : QVariant( QDateTime( QDate( value->year(), value->month(), value->day() ),
             QTime( value->hour(), value->minute(), value->second(), value->milliseconds() ) ) );
    }
    case SQLDataTypes::VarChar:
    case SQLDataTypes::LongVarChar:
    {
      String value = mResultSet->getString( columnIndex );
      return value.isNull() ? QVariant( QVariant::String ) : QVariant( QString::fromUtf8( value->c_str() ) );
    }
    case SQLDataTypes::WVarChar:
    case SQLDataTypes::WLongVarChar:
    {
      NString value = mResultSet->getNString( columnIndex );
      return value.isNull() ? QVariant( QVariant::String ) : QVariant( QString::fromStdU16String( value->c_str() ) );
    }
    case SQLDataTypes::Binary:
    case SQLDataTypes::VarBinary:
    case SQLDataTypes::LongVarBinary:
    case 29812: /* ST_GEOMETRY, ST_POINT in SAP HANA*/
    {
      Binary value = mResultSet->getBinary( columnIndex );
      if ( value.isNull() )
        return QVariant( QVariant::ByteArray );

      if ( value->size() > static_cast<size_t>( std::numeric_limits<int>::max() ) )
        throw std::runtime_error( "Binary size is larger than maximum integer value" );

      return QByteArray( value->data(), static_cast<int>( value->size() ) );
    }
    default:
      QgsDebugError( QStringLiteral( "Unhandled ODBC type %1" ).arg( QString::fromStdU16String( mMetadata->getColumnTypeName( columnIndex ) ) ) );
      return QVariant();
  }
}

QgsGeometry QgsOdbcResultSet::getGeometry( unsigned short columnIndex )
{
  auto toWkbSize = []( size_t size )
  {
    if ( size > static_cast<size_t>( std::numeric_limits<int>::max() ) )
      throw std::runtime_error( "Geometry size is larger than maximum integer value" );
    return  static_cast<int>( size );
  };

  const size_t bufLength = mResultSet->getBinaryLength( columnIndex );
  if ( bufLength == ResultSet::UNKNOWN_LENGTH )
  {
    Binary wkb = mResultSet->getBinary( columnIndex );
    if ( !wkb.isNull() && wkb->size() > 0 )
    {
      const QByteArray wkbBytes( wkb->data(), toWkbSize( wkb->size() ) );
      QgsGeometry geom;
      geom.fromWkb( wkbBytes );
      return geom;
    }
  }
  else if ( bufLength != 0 && bufLength != ResultSet::NULL_DATA )
  {
    QByteArray wkbBytes( toWkbSize( bufLength ), '0' );
    mResultSet->getBinaryData( columnIndex, wkbBytes.data(), bufLength );
    QgsGeometry geom;
    geom.fromWkb( wkbBytes );
    return geom;
  }

  return QgsGeometry();
}

QgsOdbcResultSetMetadata QgsOdbcResultSet::getMetadata()
{
  return QgsOdbcResultSetMetadata( mResultSet->getMetaDataUnicode() );
}
