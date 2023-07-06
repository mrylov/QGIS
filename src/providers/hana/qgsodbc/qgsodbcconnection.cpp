#include "qgsodbcconnection.h"
#include "qgsodbcdatabasemetadata.h"
#include "qgsodbcpreparedstatement.h"
#include "qgsodbcstatement.h"
#include "odbc/DatabaseMetaDataUnicode.h"
#include "odbc/ParameterMetaData.h"
#include "odbc/PreparedStatement.h"
#include "odbc/Statement.h"
#include <QString>

using namespace NS_ODBC;

QgsOdbcConnection::QgsOdbcConnection( QgsOdbcConnection &&conn )
  : mConnection( std::move( conn.mConnection ) )
{
}

QgsOdbcConnection::QgsOdbcConnection( std::unique_ptr<QgsOdbcConnection> &&conn )
  : mConnection( std::move( conn->mConnection ) )
{
}

QgsOdbcConnection::QgsOdbcConnection( NS_ODBC::ConnectionRef &&conn )
  : mConnection( std::move( conn ) )
{
}

void QgsOdbcConnection::connect( const QString &dsn, const QString &user, const QString &password )
{
  mConnection->connect( dsn.toStdString().c_str(), user.toStdString().c_str(), password.toStdString().c_str() );
}

void QgsOdbcConnection::connect( const QString &connString )
{
  mConnection->connect( connString.toStdString().c_str() );
}

void QgsOdbcConnection::disconnect()
{
  mConnection->disconnect();
}

void QgsOdbcConnection::commit()
{
  mConnection->commit();
}

void QgsOdbcConnection::rollback()
{
  mConnection->rollback();
}

bool QgsOdbcConnection::connected() const
{
  return mConnection->connected();
}

void QgsOdbcConnection::setAutoCommit( bool autoCommit )
{
  mConnection->setAutoCommit( autoCommit );
}

QgsOdbcStatement QgsOdbcConnection::createStatement()
{
  return QgsOdbcStatement( mConnection->createStatement() );
}

QgsOdbcPreparedStatement QgsOdbcConnection::prepareStatement( const QString &sql )
{
  return QgsOdbcPreparedStatement( mConnection->prepareStatement( reinterpret_cast<const char16_t *>( sql.utf16() ) ) );
}

QgsOdbcDatabaseMetadata QgsOdbcConnection::getDatabaseMetadata()
{
  return QgsOdbcDatabaseMetadata( mConnection->getDatabaseMetaDataUnicode() );
}
