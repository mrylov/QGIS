#include "qgsodbcconnection.h"
#include "qgsodbcenvironment.h"
#include "odbc/Connection.h"
#include <QString>
#include <QStringList>

using namespace NS_ODBC;

QgsOdbcEnvironment::QgsOdbcEnvironment()
  : mEnv( Environment::create() )
{
}

QgsOdbcConnection QgsOdbcEnvironment::createConnection()
{
  return QgsOdbcConnection( mEnv->createConnection() );
}

QStringList QgsOdbcEnvironment::getDataSources()
{
  QStringList list;
  for ( const DataSourceInformation &ds : mEnv->getDataSources() )
    list << QString::fromStdString( ds.name );
  return list;
}

bool QgsOdbcEnvironment::isDriverInstalled( const QString &name )
{
  return mEnv->isDriverInstalled( name.toStdString().c_str() );
}

void QgsOdbcEnvironment::iterateDrivers( const std::function<void( const QString &name, const QString &path )> &callback )
{
  const std::vector<DriverInformation> drivers = mEnv->getDrivers();
  for ( const DriverInformation &drv : drivers )
  {
    for ( const DriverInformation::Attribute &attr : drv.attributes )
    {
      callback( QString::fromStdString( attr.name ), QString::fromStdString( attr.value ) );
    }
  }
}
