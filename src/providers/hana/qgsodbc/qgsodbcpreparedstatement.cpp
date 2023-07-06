#include "qgsgeometry.h"
#include "qgsodbcpreparedstatement.h"
#include "qgsodbcresultset.h"
#include "qgsodbcresultsetmetadata.h"
#include "odbc/ParameterMetaData.h"
#include "odbc/PreparedStatement.h"
#include "odbc/ResultSet.h"
#include "odbc/ResultSetMetaDataUnicode.h"
#include <QByteArray>
#include <QDate>
#include <QDateTime>
#include <QString>
#include <QTime>
#include <QVariant>
#include <vector>

using namespace NS_ODBC;

QgsOdbcPreparedStatement::QgsOdbcPreparedStatement( PreparedStatementRef &&stmt )
  : mStatement( std::move( stmt ) )
{
}

QgsOdbcPreparedStatement::QgsOdbcPreparedStatement( QgsOdbcPreparedStatement &&stmt )
  : mStatement( std::move( stmt.mStatement ) )
{
  stmt.mParameterMetadata.reset();
}

void QgsOdbcPreparedStatement::addBatch()
{
  mStatement->addBatch();
}

void QgsOdbcPreparedStatement::clearBatch()
{
  mStatement->clearBatch();
}

void QgsOdbcPreparedStatement::clearParameters()
{
  mStatement->clearParameters();
}

void QgsOdbcPreparedStatement::executeBatch()
{
  mStatement->executeBatch();
}

size_t QgsOdbcPreparedStatement::executeUpdate()
{
  return mStatement->executeUpdate();
}

QgsOdbcResultSet QgsOdbcPreparedStatement::executeQuery()
{
  return QgsOdbcResultSet( mStatement->executeQuery() );
}

std::size_t QgsOdbcPreparedStatement::getBatchDataSize() const
{
  return mStatement->getBatchDataSize();
}

QgsOdbcResultSetMetadata QgsOdbcPreparedStatement::getMetadata()
{
  return QgsOdbcResultSetMetadata( mStatement->getMetaDataUnicode() );
}

void QgsOdbcPreparedStatement::setBoolean( unsigned short paramIndex, const QgsOdbcNullable<bool> &value )
{
  mStatement->setBoolean( paramIndex, value );
}

void QgsOdbcPreparedStatement::setByte( unsigned short paramIndex, const QgsOdbcNullable<std::int8_t> &value )
{
  mStatement->setByte( paramIndex, value );
}

void QgsOdbcPreparedStatement::setUByte( unsigned short paramIndex, const QgsOdbcNullable<std::uint8_t> &value )
{
  mStatement->setUByte( paramIndex, value );
}

void QgsOdbcPreparedStatement::setShort( unsigned short paramIndex, const QgsOdbcNullable<std::int16_t> &value )
{
  mStatement->setShort( paramIndex, value );
}

void QgsOdbcPreparedStatement::setUShort( unsigned short paramIndex, const QgsOdbcNullable<std::uint16_t> &value )
{
  mStatement->setUShort( paramIndex, value );
}

void QgsOdbcPreparedStatement::setInt( unsigned short paramIndex, const QgsOdbcNullable<std::int32_t> &value )
{
  mStatement->setInt( paramIndex, value );
}

void QgsOdbcPreparedStatement::setUInt( unsigned short paramIndex, const QgsOdbcNullable<std::uint32_t> &value )
{
  mStatement->setUInt( paramIndex, value );
}

void QgsOdbcPreparedStatement::setLong( unsigned short paramIndex, const QgsOdbcNullable<std::int64_t> &value )
{
  mStatement->setLong( paramIndex, value );
}

void QgsOdbcPreparedStatement::setULong( unsigned short paramIndex, const QgsOdbcNullable<std::uint64_t> &value )
{
  mStatement->setULong( paramIndex, value );
}

void QgsOdbcPreparedStatement::setFloat( unsigned short paramIndex, const QgsOdbcNullable<float> &value )
{
  mStatement->setFloat( paramIndex, value );
}

void QgsOdbcPreparedStatement::setDouble( unsigned short paramIndex, const QgsOdbcNullable<double> &value )
{
  mStatement->setDouble( paramIndex, value );
}

void QgsOdbcPreparedStatement::setString( unsigned short paramIndex, const QString &value )
{
  if ( value.isNull() )
    mStatement->setCString( paramIndex, nullptr, 0 );
  else
  {
    auto str = value.toStdString();
    mStatement->setCString( paramIndex, str.c_str(), str.length() );
  }
}

void QgsOdbcPreparedStatement::setNString( unsigned short paramIndex, const QString &value )
{
  if ( value.isNull() )
    mStatement->setNCString( paramIndex, nullptr, 0 );
  else
  {
    auto str = value.toStdU16String();
    mStatement->setNCString( paramIndex, str.c_str(), str.length() );
  }
}

void QgsOdbcPreparedStatement::setBinary( unsigned short paramIndex, const QByteArray &value )
{
  if ( value.isNull() )
    mStatement->setBinary( paramIndex, Binary() );
  else
    mStatement->setBinary( paramIndex, Binary( std::vector<char>( value.begin(), value.end() ) ) );
}

void QgsOdbcPreparedStatement::setDate( unsigned short paramIndex, const QDate &value )
{
  if ( value.isNull() )
    mStatement->setDate( paramIndex, Date() );
  else
    mStatement->setDate( paramIndex, makeNullable<date>( value.year(), value.month(), value.day() ) );
}

void QgsOdbcPreparedStatement::setDateTime( unsigned short paramIndex, const QDateTime &value )
{
  if ( value.isNull() )
    mStatement->setTimestamp( paramIndex, Timestamp() );
  else
  {
    QDate d = value.date();
    QTime t = value.time();
    mStatement->setTimestamp( paramIndex,  makeNullable<NS_ODBC::timestamp>( d.year(),
                              d.month(), d.day(), t.hour(), t.minute(), t.second(), t.msec() ) );
  }
}

void QgsOdbcPreparedStatement::setTime( unsigned short paramIndex, const QTime &value )
{
  if ( value.isNull() )
    mStatement->setTime( paramIndex, Time() );
  else
    mStatement->setTime( paramIndex,  makeNullable<NS_ODBC::time>( value.hour(), value.minute(), value.second() ) );
}

void QgsOdbcPreparedStatement::setGeometry( unsigned short paramIndex, const QgsGeometry &value )
{
  if ( value.isNull() )
    mStatement->setBinary( paramIndex, Binary() );
  else
  {
    QByteArray wkb = value.asWkb();
    mStatement->setBinary( paramIndex, makeNullable<std::vector<char>>( wkb.begin(), wkb.end() ) );
  }
}

void QgsOdbcPreparedStatement::setVariant( unsigned short paramIndex, const QVariant &value )
{
  if ( mParameterMetadata.isNull() )
    mParameterMetadata = mStatement->getParameterMetaData();

  short type = mParameterMetadata->getParameterType( paramIndex );
  switch ( type )
  {
    case SQLDataTypes::Bit:
    case SQLDataTypes::Boolean:
      mStatement->setBoolean( paramIndex, value.isNull() ? Boolean() : Boolean( value.toBool() ) );
      break;
    case SQLDataTypes::TinyInt:
      if ( value.type() == QVariant::Int )
        mStatement->setByte( paramIndex, value.isNull() ? Byte() : Byte( static_cast<int8_t>( value.toInt() ) ) );
      else
        mStatement->setUByte( paramIndex, value.isNull() ? UByte() : UByte( static_cast<uint8_t>( value.toUInt() ) ) );
      break;
    case SQLDataTypes::SmallInt:
      if ( value.type() == QVariant::Int )
        mStatement->setShort( paramIndex, value.isNull() ? Short() : Short( static_cast<int16_t>( value.toInt() ) ) );
      else
        mStatement->setUShort( paramIndex, value.isNull() ? UShort() : UShort( static_cast<uint16_t>( value.toUInt() ) ) );
      break;
    case SQLDataTypes::Integer:
      if ( value.type() == QVariant::Int )
        mStatement->setInt( paramIndex, value.isNull() ? Int() : Int( value.toInt() ) );
      else
        mStatement->setUInt( paramIndex, value.isNull() ? UInt() : UInt( value.toUInt() ) );
      break;
    case SQLDataTypes::BigInt:
      if ( value.type() == QVariant::LongLong )
        mStatement->setLong( paramIndex, value.isNull()  ? Long() : Long( value.toLongLong() ) );
      else
        mStatement->setULong( paramIndex, value.isNull()  ? ULong() : ULong( value.toULongLong() ) );
      break;
    case SQLDataTypes::Numeric:
    case SQLDataTypes::Decimal:
      mStatement->setDouble( paramIndex, value.isNull()  ? Double() : Double( value.toDouble() ) );
      break;
    case SQLDataTypes::Real:
      mStatement->setFloat( paramIndex, value.isNull() ? Float() : Float( value.toFloat() ) );
      break;
    case SQLDataTypes::Float:
    case SQLDataTypes::Double:
      mStatement->setDouble( paramIndex, value.isNull() ? Double() : Double( value.toDouble() ) );
      break;
    case SQLDataTypes::Date:
    case SQLDataTypes::TypeDate:
      if ( value.isNull() )
        mStatement->setDate( paramIndex, Date() );
      else
      {
        QDate d = value.toDate();
        mStatement->setDate( paramIndex, makeNullable<date>( d.year(), d.month(), d.day() ) );
      }
      break;
    case SQLDataTypes::Time:
    case SQLDataTypes::TypeTime:
      if ( value.isNull() )
        mStatement->setTime( paramIndex, Time() );
      else
      {
        QTime t = value.toTime();
        mStatement->setTime( paramIndex, makeNullable<NS_ODBC::time>( t.hour(), t.minute(), t.second() ) );
      }
      break;
    case SQLDataTypes::Timestamp:
    case SQLDataTypes::TypeTimestamp:
      if ( value.isNull() )
        mStatement->setTimestamp( paramIndex, Timestamp() );
      else
      {
        QDateTime dt = value.toDateTime();
        QDate d = dt.date();
        QTime t = dt.time();
        mStatement->setTimestamp( paramIndex, makeNullable<timestamp>( d.year(),
                                  d.month(), d.day(), t.hour(), t.minute(), t.second(), t.msec() ) );
      }
      break;
    case SQLDataTypes::Char:
    case SQLDataTypes::VarChar:
    case SQLDataTypes::LongVarChar:
      mStatement->setString( paramIndex,   value.isNull() ? String() : String( value.toString().toStdString() ) );
      break;
    case SQLDataTypes::WChar:
    case SQLDataTypes::WVarChar:
    case SQLDataTypes::WLongVarChar:
      mStatement->setNString( paramIndex,   value.isNull() ? NString() : NString( value.toString().toStdU16String() ) );
      break;
    case SQLDataTypes::Binary:
    case SQLDataTypes::VarBinary:
    case SQLDataTypes::LongVarBinary:
      if ( value.isNull() )
        mStatement->setBinary( paramIndex, Binary() );
      else
      {
        QByteArray arr = value.toByteArray();
        mStatement->setBinary( paramIndex, Binary( std::vector<char>( arr.begin(), arr.end() ) ) );
      }
      break;
    default:
      QString msg = QStringLiteral( "Unknown value type ('%1') for parameter %2" )
                    .arg( QString::number( type ), QString::number( paramIndex ) ) ;
      throw std::runtime_error( msg.toStdString().c_str() );
  }
}
