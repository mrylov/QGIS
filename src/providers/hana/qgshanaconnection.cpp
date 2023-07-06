/***************************************************************************
   qgshanaconnection.cpp
   --------------------------------------
   Date      : 31-05-2019
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
#include "qgsexception.h"
#include "qgsdatasourceuri.h"
#include "qgshanaconnection.h"
#include "qgshanaconnectionstringbuilder.h"
#include "qgshanadriver.h"
#include "qgshanaexception.h"
#include "qgshanatablemodel.h"
#include "qgshanautils.h"
#include "qgslogger.h"
#include "qgsmessagelog.h"
#include "qgscredentials.h"
#include "qgssettings.h"
#include "qgsvariantutils.h"

#include "qgsodbc/qgsodbcdatabasemetadata.h"
#include "qgsodbc/qgsodbcpreparedstatement.h"
#include "qgsodbc/qgsodbcresultset.h"
#include "qgsodbc/qgsodbcresultsetmetadata.h"
#include "qgsodbc/qgsodbcstatement.h"

using namespace std;

namespace
{
  QMap<QString, bool> getColumnsUniqueness( QgsOdbcConnection &conn, const QString &schemaName, const QString &tableName )
  {
    QMap<QString, bool> ret;
    QgsOdbcDatabaseMetadata dmd = conn.getDatabaseMetadata();
    QgsOdbcResultSet rsStats = dmd.getStatistics( QString(), schemaName, tableName, QgsOdbcIndexType::UNIQUE, QgsOdbcStatisticsAccuracy::ENSURE );
    QMap<QString, QStringList> compositeKeys;
    while ( rsStats.next() )
    {
      QString clmName = rsStats.getString( 9 /*COLUMN_NAME*/ );
      if ( clmName.isEmpty() )
        continue;
      bool unique = *rsStats.getShort( 4 /*NON_UNIQUE*/ ) == 0;
      QString indexName = rsStats.getString( 6 /*INDEX_NAME*/ );
      ret.insert( clmName, unique );
      compositeKeys[indexName].append( clmName );
    }
    rsStats.close();

    for ( const QStringList &indexColumns : compositeKeys )
    {
      if ( indexColumns.size() <= 1 )
        continue;
      for ( const QString &clmName : indexColumns )
        ret[clmName] = false;
    }

    return ret;
  }
}

QgsField AttributeField::toQgsField() const
{
  QVariant::Type fieldType;
  switch ( type )
  {
    case QgsOdbcDataTypes::Bit:
    case QgsOdbcDataTypes::Boolean:
      fieldType = QVariant::Bool;
      break;
    case QgsOdbcDataTypes::TinyInt:
    case QgsOdbcDataTypes::SmallInt:
    case QgsOdbcDataTypes::Integer:
      fieldType = isSigned ? QVariant::Int : QVariant::UInt;
      break;
    case QgsOdbcDataTypes::BigInt:
      fieldType = isSigned ? QVariant::LongLong : QVariant::ULongLong;
      break;
    case QgsOdbcDataTypes::Numeric:
    case QgsOdbcDataTypes::Decimal:
      fieldType = QVariant::Double;
      break;
    case QgsOdbcDataTypes::Double:
    case QgsOdbcDataTypes::Float:
    case QgsOdbcDataTypes::Real:
      fieldType = QVariant::Double;
      break;
    case QgsOdbcDataTypes::Char:
    case QgsOdbcDataTypes::WChar:
      fieldType = ( size == 1 ) ? QVariant::Char : QVariant::String;
      break;
    case QgsOdbcDataTypes::VarChar:
    case QgsOdbcDataTypes::WVarChar:
    case QgsOdbcDataTypes::LongVarChar:
    case QgsOdbcDataTypes::WLongVarChar:
      fieldType = QVariant::String;
      break;
    case QgsOdbcDataTypes::Binary:
    case QgsOdbcDataTypes::VarBinary:
    case QgsOdbcDataTypes::LongVarBinary:
      fieldType = QVariant::ByteArray;
      break;
    case QgsOdbcDataTypes::Date:
    case QgsOdbcDataTypes::TypeDate:
      fieldType = QVariant::Date;
      break;
    case QgsOdbcDataTypes::Time:
    case QgsOdbcDataTypes::TypeTime:
      fieldType = QVariant::Time;
      break;
    case QgsOdbcDataTypes::Timestamp:
    case QgsOdbcDataTypes::TypeTimestamp:
      fieldType = QVariant::DateTime;
      break;
    default:
      if ( isGeometry() )
        // There are two options how to treat geometry columns that are attributes:
        // 1. Type is QVariant::String. The value is provided as WKT and editable.
        // 2. Type is QVariant::ByteArray. The value is provided as BLOB and uneditable.
        fieldType = QVariant::String;
      else
        throw QgsHanaException( QString( "Field type '%1' is not supported" ).arg( QString::number( type ) ) );
      break;
  }

  QgsField field = QgsField( name, fieldType, typeName, size, precision, comment, QVariant::Invalid );
  if ( !isNullable || isUnique )
  {
    QgsFieldConstraints constraints;
    if ( !isNullable )
      constraints.setConstraint( QgsFieldConstraints::ConstraintNotNull, QgsFieldConstraints::ConstraintOriginProvider );
    if ( isUnique )
      constraints.setConstraint( QgsFieldConstraints::ConstraintUnique, QgsFieldConstraints::ConstraintOriginProvider );
    field.setConstraints( constraints );
  }
  return field;
}

void setPreparedStatementValues( QgsOdbcPreparedStatement &stmt, const QVariantList &args )
{
  if ( args.isEmpty() )
    return;


  for ( unsigned short i = 1; i <= args.size(); ++i )
  {
    const QVariant &value = args.at( i - 1 );
    switch ( value.type() )
    {
      case QVariant::Type::Double:
        stmt.setDouble( i, value.isNull() ? QgsOdbcNullable<double>() : value.toDouble() );
        break;
      case QVariant::Type::Int:
        stmt.setInt( i, value.isNull() ?  QgsOdbcNullable<int32_t>() : value.toInt() );
        break;
      case QVariant::Type::UInt:
        stmt.setUInt( i, value.isNull() ?  QgsOdbcNullable<uint32_t>() : value.toUInt() );
        break;
      case QVariant::Type::LongLong:
        stmt.setLong( i, value.isNull() ? QgsOdbcNullable<int64_t>() : value.toLongLong() );
        break;
      case QVariant::Type::ULongLong:
        stmt.setULong( i, value.isNull() ? QgsOdbcNullable<uint64_t>() : value.toULongLong() );
        break;
      case QVariant::Type::String:
        stmt.setNString( i, value.toString() );
        break;
      default:
        throw QgsHanaException( "Parameter type is not supported" );
    }
  }
}

int getSrid( QgsOdbcResultSet &rsSrid )
{
  int srid = -1;
  while ( rsSrid.next() )
  {
    QgsOdbcNullable<int32_t> value = rsSrid.getInt( 1 );
    if ( value.isNull() )
      continue;
    if ( srid == -1 )
      srid = *value;
    else if ( srid != *value )
    {
      srid = -1;
      break;
    }
  }
  rsSrid.close();
  return srid;
}

static const uint8_t CREDENTIALS_INPUT_MAX_ATTEMPTS = 5;
static const int GEOMETRIES_SELECT_LIMIT = 10;

QgsHanaConnection::QgsHanaConnection( QgsOdbcConnection &&connection,  const QgsDataSourceUri &uri )
  : mConnection( move( connection ) )
  , mUri( uri )
{
}

QgsHanaConnection::~QgsHanaConnection()
{
  if ( !mConnection.connected() )
    return;

  try
  {
    // The rollback needs to be called here because the driver throws an exception
    // if AutoCommit is set to false.
    mConnection.rollback();

    mConnection.disconnect();
  }
  catch ( const QgsOdbcException &ex )
  {
    QgsMessageLog::logMessage( QgsHanaUtils::formatErrorMessage( ex.what() ), tr( "SAP HANA" ) );
  }
}

QgsHanaConnection *QgsHanaConnection::createConnection( const QgsDataSourceUri &uri )
{
  return createConnection( uri, nullptr, nullptr );
}

QgsHanaConnection *QgsHanaConnection::createConnection( const QgsDataSourceUri &uri, bool *canceled )
{
  return createConnection( uri, canceled, nullptr );
}

QgsHanaConnection *QgsHanaConnection::createConnection( const QgsDataSourceUri &uri, bool *canceled, QString *errorMessage )
{
  if ( canceled != nullptr )
    *canceled = false;

  try
  {
    unique_ptr<QgsOdbcConnection> conn = make_unique<QgsOdbcConnection>( QgsHanaDriver::instance()->createConnection() );
    conn->setAutoCommit( false );
    QString message;

    auto connect = []( QgsOdbcConnection & conn,
                       const QgsDataSourceUri & uri,
                       QString & errorMessage )
    {
      try
      {
        QgsHanaConnectionStringBuilder sb( uri );
        conn.connect( sb.toString().toStdString().c_str() );
        errorMessage = QString();
      }
      catch ( const QgsOdbcException &ex )
      {
        errorMessage = QObject::tr( "Connection to database failed" ) + '\n' + QgsHanaUtils::formatErrorMessage( ex.what() );
        QgsDebugError( errorMessage );
        QgsMessageLog::logMessage( errorMessage, tr( "SAP HANA" ) );
      }

      return conn.connected();
    };

    if ( !connect( *conn, uri, message ) )
    {
      QString conninfo = uri.uri( false );
      QString username = uri.username();
      QString password = uri.password();
      QgsDataSourceUri tmpUri( uri );

      QgsCredentials::instance()->lock();

      int i = 0;
      while ( i < CREDENTIALS_INPUT_MAX_ATTEMPTS )
      {
        ++i;
        bool ok = QgsCredentials::instance()->get( conninfo, username, password, message );
        if ( !ok )
        {
          if ( canceled != nullptr )
            *canceled = true;
          break;
        }

        if ( !username.isEmpty() )
          tmpUri.setUsername( username );
        if ( !password.isEmpty() )
          tmpUri.setPassword( password );

        if ( connect( *conn, tmpUri, message ) )
          break;
      }

      QgsCredentials::instance()->put( conninfo, username, password );
      QgsCredentials::instance()->unlock();
    }

    if ( conn->connected() )
      return new QgsHanaConnection( move( conn ), uri );
    else
      throw QgsHanaException( message.toStdString().c_str() );
  }
  catch ( const QgsHanaException &ex )
  {
    if ( errorMessage != nullptr )
      *errorMessage = ex.what();
  }

  return nullptr;
}

QStringList QgsHanaConnection::connectionList()
{
  QgsSettings settings;
  settings.beginGroup( QStringLiteral( "HANA/connections" ) );
  return settings.childGroups();
}

QString QgsHanaConnection::connInfo() const
{
  return QgsHanaUtils::connectionInfo( mUri );
}

void QgsHanaConnection::execute( const QString &sql )
{
  try
  {
    QgsOdbcStatement stmt = mConnection.createStatement();
    stmt.execute( sql );
  }
  catch ( const QgsOdbcException &ex )
  {
    throw QgsHanaException( ex.what() );
  }
}

bool QgsHanaConnection::execute( const QString &sql, QString *errorMessage )
{
  try
  {
    QgsOdbcStatement stmt = mConnection.createStatement();
    stmt.execute( sql );
    mConnection.commit();
    return true;
  }
  catch ( const QgsOdbcException &ex )
  {
    if ( errorMessage )
      *errorMessage = QgsHanaUtils::formatErrorMessage( ex.what() );
  }

  return false;
}

QgsOdbcResultSet QgsHanaConnection::executeQuery( const QString &sql )
{
  try
  {
    QgsOdbcStatement stmt = mConnection.createStatement();
    return stmt.executeQuery( sql );
  }
  catch ( const QgsOdbcException &ex )
  {
    throw QgsHanaException( ex.what() );
  }
}

QgsOdbcResultSet QgsHanaConnection::executeQuery( const QString &sql, const QVariantList &args )
{
  try
  {
    QgsOdbcPreparedStatement stmt = mConnection.prepareStatement( sql );
    setPreparedStatementValues( stmt, args );
    return stmt.executeQuery();
  }
  catch ( const QgsOdbcException &ex )
  {
    throw QgsHanaException( ex.what() );
  }
}

size_t QgsHanaConnection::executeCountQuery( const QString &sql )
{
  try
  {
    QgsOdbcStatement stmt = mConnection.createStatement();
    QgsOdbcResultSet rs = stmt.executeQuery( sql );
    rs.next();
    size_t ret = static_cast<size_t>( *rs.getLong( 1 ) );
    rs.close();
    return ret;
  }
  catch ( const QgsOdbcException &ex )
  {
    throw QgsHanaException( ex.what() );
  }
}

size_t QgsHanaConnection::executeCountQuery( const QString &sql, const QVariantList &args )
{
  try
  {
    QgsOdbcPreparedStatement stmt = mConnection.prepareStatement( sql );
    setPreparedStatementValues( stmt, args );
    QgsOdbcResultSet rs = stmt.executeQuery();
    rs.next();
    size_t ret = static_cast<size_t>( *rs.getLong( 1 ) );
    rs.close();
    return ret;
  }
  catch ( const QgsOdbcException &ex )
  {
    throw QgsHanaException( ex.what() );
  }
}

QVariant QgsHanaConnection::executeScalar( const QString &sql )
{
  try
  {
    QVariant res;
    QgsOdbcStatement stmt = mConnection.createStatement();
    QgsOdbcResultSet resultSet = stmt.executeQuery( sql );
    if ( resultSet.next() )
      res = resultSet.getVariant( 1 );
    resultSet.close();
    return  res;
  }
  catch ( const QgsOdbcException &ex )
  {
    throw QgsHanaException( ex.what() );
  }
}

QVariant QgsHanaConnection::executeScalar( const QString &sql, const QVariantList &args )
{
  try
  {
    QVariant res;
    QgsOdbcPreparedStatement stmt = mConnection.prepareStatement( sql );
    setPreparedStatementValues( stmt, args );
    QgsOdbcResultSet resultSet = stmt.executeQuery( );
    if ( resultSet.next() )
      res = resultSet.getVariant( 1 );
    resultSet.close();
    return  res;
  }
  catch ( const QgsOdbcException &ex )
  {
    throw QgsHanaException( ex.what() );
  }
}

QgsOdbcPreparedStatement QgsHanaConnection::prepareStatement( const QString &sql )
{
  try
  {
    return mConnection.prepareStatement( sql );
  }
  catch ( const QgsOdbcException &ex )
  {
    throw QgsHanaException( ex.what() );
  }
}

void QgsHanaConnection::commit()
{
  try
  {
    mConnection.commit();
  }
  catch ( const QgsOdbcException &ex )
  {
    throw QgsHanaException( ex.what() );
  }
}

void QgsHanaConnection::rollback()
{
  try
  {
    mConnection.rollback();
  }
  catch ( const QgsOdbcException &ex )
  {
    throw QgsHanaException( ex.what() );
  }
}

QList<QgsVectorDataProvider::NativeType> QgsHanaConnection::getNativeTypes()
{
  return QList<QgsVectorDataProvider::NativeType>()
         // boolean
         << QgsVectorDataProvider::NativeType( QgsVariantUtils::typeToDisplayString( QVariant::Bool ), QStringLiteral( "BOOLEAN" ), QVariant::Bool, -1, -1, -1, -1 )
         // integer types
         << QgsVectorDataProvider::NativeType( tr( "8 bytes Integer" ), QStringLiteral( "BIGINT" ), QVariant::LongLong, -1, -1, 0, 0 )
         << QgsVectorDataProvider::NativeType( tr( "4 bytes Integer" ), QStringLiteral( "INTEGER" ), QVariant::Int, -1, -1, 0, 0 )
         << QgsVectorDataProvider::NativeType( tr( "2 bytes Integer" ), QStringLiteral( "SMALLINT" ), QVariant::Int, -1, -1, 0, 0 )
         << QgsVectorDataProvider::NativeType( tr( "1 byte Integer" ), QStringLiteral( "TINYINT" ), QVariant::Int, -1, -1, 0, 0 )
         << QgsVectorDataProvider::NativeType( tr( "Decimal Number (DECIMAL)" ), QStringLiteral( "DECIMAL" ), QVariant::Double, 1, 31, 0, 31 )
         // floating point
         << QgsVectorDataProvider::NativeType( tr( "Decimal Number (REAL)" ), QStringLiteral( "REAL" ), QVariant::Double )
         << QgsVectorDataProvider::NativeType( tr( "Decimal Number (DOUBLE)" ), QStringLiteral( "DOUBLE" ), QVariant::Double )
         // date/time types
         << QgsVectorDataProvider::NativeType( QgsVariantUtils::typeToDisplayString( QVariant::Date ), QStringLiteral( "DATE" ), QVariant::Date, -1, -1, -1, -1 )
         << QgsVectorDataProvider::NativeType( QgsVariantUtils::typeToDisplayString( QVariant::Time ), QStringLiteral( "TIME" ), QVariant::Time, -1, -1, -1, -1 )
         << QgsVectorDataProvider::NativeType( QgsVariantUtils::typeToDisplayString( QVariant::DateTime ), QStringLiteral( "TIMESTAMP" ), QVariant::DateTime, -1, -1, -1, -1 )
         // string types
         << QgsVectorDataProvider::NativeType( tr( "Text, variable length (VARCHAR)" ), QStringLiteral( "VARCHAR" ), QVariant::String, 1, 5000 )
         << QgsVectorDataProvider::NativeType( tr( "Unicode Text, variable length (NVARCHAR)" ), QStringLiteral( "NVARCHAR" ), QVariant::String, 1, 5000 )
         << QgsVectorDataProvider::NativeType( tr( "Text, variable length large object (CLOB)" ), QStringLiteral( "CLOB" ), QVariant::String )
         << QgsVectorDataProvider::NativeType( tr( "Unicode Text, variable length large object (NCLOB)" ), QStringLiteral( "NCLOB" ), QVariant::String )
         // binary types
         << QgsVectorDataProvider::NativeType( tr( "Binary Object (VARBINARY)" ), QStringLiteral( "VARBINARY" ), QVariant::ByteArray, 1, 5000 )
         << QgsVectorDataProvider::NativeType( tr( "Binary Object (BLOB)" ), QStringLiteral( "BLOB" ), QVariant::ByteArray );
}

const QString &QgsHanaConnection::getDatabaseVersion()
{
  if ( mDatabaseVersion.isEmpty() )
  {
    try
    {
      QgsOdbcDatabaseMetadata dbmd = mConnection.getDatabaseMetadata();
      mDatabaseVersion = dbmd.getDBMSVersion();
    }
    catch ( const QgsOdbcException &ex )
    {
      throw QgsHanaException( ex.what() );
    }
  }

  return mDatabaseVersion;
}

const QString &QgsHanaConnection::getUserName()
{
  if ( mUserName.isEmpty() )
    mUserName = executeScalar( QStringLiteral( "SELECT CURRENT_USER FROM DUMMY" ) ).toString();

  return mUserName;
}

QgsCoordinateReferenceSystem QgsHanaConnection::getCrs( int srid )
{
  QgsCoordinateReferenceSystem crs;
  const char *sql = "SELECT ORGANIZATION, ORGANIZATION_COORDSYS_ID, DEFINITION, TRANSFORM_DEFINITION FROM SYS.ST_SPATIAL_REFERENCE_SYSTEMS WHERE SRS_ID = ?";

  try
  {
    QgsOdbcPreparedStatement stmt = mConnection.prepareStatement( sql );
    stmt.setInt( 1, QgsOdbcNullable<int32_t>( srid ) );
    QgsOdbcResultSet rsSrs = stmt.executeQuery();
    if ( rsSrs.next() )
    {
      auto organization = rsSrs.getString( 1 );
      if ( !organization.isNull() )
      {
        QString srid = QStringLiteral( "%1:%2" ).arg( organization.toLower(), QString::number( *rsSrs.getInt( 2 ) ) );
        crs.createFromString( srid );
      }

      if ( !crs.isValid() )
      {
        auto wkt = rsSrs.getString( 3 );
        if ( !wkt.isNull() )
          crs = QgsCoordinateReferenceSystem::fromWkt( wkt );

        if ( !crs.isValid() )
        {
          auto proj = rsSrs.getString( 4 );
          if ( !proj.isNull() )
            crs = QgsCoordinateReferenceSystem::fromProj( proj );
        }
      }
    }
    rsSrs.close();
  }
  catch ( const QgsOdbcException &ex )
  {
    throw QgsHanaException( ex.what() );
  }

  return crs;
}

QVector<QgsHanaLayerProperty> QgsHanaConnection::getLayers(
  const QString &schemaName,
  bool allowGeometrylessTables,
  bool userTablesOnly,
  const function<bool( const QgsHanaLayerProperty &layer )> &layerFilter )
{
  const QString schema = mUri.schema().isEmpty() ? schemaName : mUri.schema();
  const QString sqlSchemaFilter = QStringLiteral(
                                    "SELECT DISTINCT(SCHEMA_NAME) FROM SYS.EFFECTIVE_PRIVILEGES WHERE "
                                    "OBJECT_TYPE IN ('SCHEMA', 'TABLE', 'VIEW') AND "
                                    "SCHEMA_NAME LIKE ? AND "
                                    "SCHEMA_NAME NOT LIKE_REGEXPR 'SYS|_SYS.*|UIS|SAP_XS|SAP_REST|HANA_XS' AND "
                                    "PRIVILEGE IN ('SELECT', 'CREATE ANY') AND "
                                    "USER_NAME = CURRENT_USER AND IS_VALID = 'TRUE'" );

  const QString sqlOwnerFilter = userTablesOnly ? QStringLiteral( "OWNER_NAME = CURRENT_USER" ) : QStringLiteral( "OWNER_NAME IS NOT NULL" );

  const QString sqlDataTypeFilter = !allowGeometrylessTables ? QStringLiteral( "DATA_TYPE_NAME IN ('ST_GEOMETRY','ST_POINT')" ) :
                                    QStringLiteral( "DATA_TYPE_NAME IS NOT NULL" );

  const QString sqlTables = QStringLiteral(
                              "SELECT SCHEMA_NAME, TABLE_NAME, COLUMN_NAME, DATA_TYPE_NAME, TABLE_COMMENTS FROM "
                              "(SELECT * FROM SYS.TABLE_COLUMNS WHERE "
                              "TABLE_OID IN (SELECT OBJECT_OID FROM OWNERSHIP WHERE OBJECT_TYPE = 'TABLE' AND %1) AND "
                              "SCHEMA_NAME IN (%2) AND %3) "
                              "INNER JOIN "
                              "(SELECT TABLE_OID AS TABLE_OID_2, COMMENTS AS TABLE_COMMENTS FROM SYS.TABLES WHERE IS_USER_DEFINED_TYPE = 'FALSE') "
                              "ON TABLE_OID = TABLE_OID_2" );

  const QString sqlViews = QStringLiteral(
                             "SELECT SCHEMA_NAME, VIEW_NAME, COLUMN_NAME, DATA_TYPE_NAME, VIEW_COMMENTS FROM "
                             "(SELECT * FROM SYS.VIEW_COLUMNS WHERE "
                             "VIEW_OID IN (SELECT OBJECT_OID FROM OWNERSHIP WHERE OBJECT_TYPE = 'VIEW' AND %1) AND "
                             "SCHEMA_NAME IN (%2) AND %3) "
                             "INNER JOIN "
                             "(SELECT VIEW_OID AS VIEW_OID_2, COMMENTS AS VIEW_COMMENTS FROM SYS.VIEWS) "
                             "ON VIEW_OID = VIEW_OID_2" );

  QMultiHash<QPair<QString, QString>, QgsHanaLayerProperty> layers;

  auto addLayers = [&]( const QString & sql, bool isView )
  {
    QgsOdbcPreparedStatement stmt = prepareStatement( sql );
    stmt.setNString( 1, schema.isEmpty() ? QStringLiteral( "%" ) : schema );
    QgsOdbcResultSet rsLayers =  stmt.executeQuery();

    while ( rsLayers.next() )
    {
      QgsHanaLayerProperty layer;
      layer.schemaName = rsLayers.getString( 1 );
      layer.tableName = rsLayers.getString( 2 );
      QString geomColumnType = rsLayers.getString( 4 );
      bool isGeometryColumn = ( geomColumnType == QLatin1String( "ST_GEOMETRY" ) || geomColumnType == QLatin1String( "ST_POINT" ) );
      layer.geometryColName = isGeometryColumn ? rsLayers.getString( 3 ) : QString();
      layer.tableComment = rsLayers.getString( 5 );
      layer.isView = isView;
      layer.srid = -1;
      layer.type = isGeometryColumn ? Qgis::WkbType::Unknown : Qgis::WkbType::NoGeometry;

      if ( layerFilter && !layerFilter( layer ) )
        continue;

      QPair<QString, QString> layerKey( layer.schemaName, layer.tableName );
      if ( allowGeometrylessTables )
      {
        int layersCount = layers.count( layerKey );
        if ( !isGeometryColumn && layersCount >= 1 )
          continue;
        if ( layersCount == 1 )
        {
          QgsHanaLayerProperty firstLayer = layers.values( layerKey ).value( 0 );
          if ( firstLayer.geometryColName.isEmpty() )
            layers.remove( layerKey );
        }
      }

      layers.insert( layerKey, layer );
    }
    rsLayers.close();
  };

  try
  {
    QString sql = sqlTables.arg( sqlOwnerFilter, sqlSchemaFilter, sqlDataTypeFilter );
    addLayers( sql, false );

    sql = sqlViews.arg( sqlOwnerFilter, sqlSchemaFilter, sqlDataTypeFilter );
    addLayers( sql, true );
  }
  catch ( const QgsOdbcException &ex )
  {
    throw QgsHanaException( ex.what() );
  }

  QVector<QgsHanaLayerProperty> list;
  const auto uniqueKeys = layers.uniqueKeys();
  for ( const QPair<QString, QString> &key : uniqueKeys )
  {
    QList<QgsHanaLayerProperty> values = layers.values( key );
    if ( values.size() == 1 )
      values[0].isUnique = true;

    for ( const QgsHanaLayerProperty &lp : std::as_const( values ) )
      list << lp;
  }

  return list;
}

QVector<QgsHanaLayerProperty> QgsHanaConnection::getLayersFull(
  const QString &schemaName,
  bool allowGeometrylessTables,
  bool userTablesOnly,
  const function<bool( const QgsHanaLayerProperty &layer )> &layerFilter )
{
  QVector<QgsHanaLayerProperty> layers = getLayers( schemaName, allowGeometrylessTables, userTablesOnly, layerFilter );
  // We cannot use a range-based for loop as layers are modified in readLayerInfo.
  for ( int i = 0; i < layers.size(); ++i )
    readLayerInfo( layers[i] );
  return layers;
}

void QgsHanaConnection::readLayerInfo( QgsHanaLayerProperty &layerProperty )
{
  try
  {
    layerProperty.srid = getColumnSrid( layerProperty.schemaName, layerProperty.tableName, layerProperty.geometryColName );
    layerProperty.type = getColumnGeometryType( layerProperty.schemaName, layerProperty.tableName, layerProperty.geometryColName );
    layerProperty.pkCols = getPrimaryKeyCandidates( layerProperty );
    layerProperty.isValid = true;
  }
  catch ( const QgsHanaException &ex )
  {
    layerProperty.errorMessage = ex.what();
    QgsMessageLog::logMessage( QgsHanaUtils::formatErrorMessage( ex.what() ), tr( "SAP HANA" ) );
  }
}

void QgsHanaConnection::readQueryFields( const QString &schemaName, const QString &sql,
    const function<void( const AttributeField &field )> &callback )
{
  QMap<QString, QMap<QString, QString>> clmComments;
  auto getColumnComments = [&clmComments, &conn = mConnection](
                             const QString & schemaName, const QString & tableName, const QString & columnName )
  {
    if ( schemaName.isEmpty() || tableName.isEmpty() || columnName.isEmpty() )
      return QString();
    const QString key = QStringLiteral( "%1.%2" ).arg( QgsHanaUtils::quotedIdentifier( schemaName ), QgsHanaUtils::quotedIdentifier( tableName ) );
    if ( !clmComments.contains( key ) )
    {
      const char *sql = "SELECT COLUMN_NAME, COMMENTS FROM SYS.TABLE_COLUMNS WHERE SCHEMA_NAME = ? AND TABLE_NAME = ?";
      QgsOdbcPreparedStatement stmt = conn.prepareStatement( sql );
      stmt.setNString( 1, schemaName );
      stmt.setNString( 2, tableName );

      QgsOdbcResultSet rsColumns = stmt.executeQuery();
      while ( rsColumns.next() )
      {
        QString name = rsColumns.getString( 1 );
        QString comments = rsColumns.getString( 2 );
        clmComments[key].insert( name, comments );
      }
      rsColumns.close();
    }
    return clmComments[key].value( columnName );
  };

  QMap<QString, QMap<QString, bool>> clmUniqueness;
  auto isColumnUnique = [&clmUniqueness, &conn = mConnection](
                          const QString & schemaName, const QString & tableName, const QString & columnName )
  {
    if ( schemaName.isEmpty() || tableName.isEmpty() || columnName.isEmpty() )
      return false;
    const QString key = QStringLiteral( "%1.%2" ).arg( QgsHanaUtils::quotedIdentifier( schemaName ), QgsHanaUtils::quotedIdentifier( tableName ) );
    if ( !clmUniqueness.contains( key ) )
      clmUniqueness.insert( key, getColumnsUniqueness( conn, schemaName, tableName ) );
    return clmUniqueness[key].value( columnName, false );
  };

  try
  {
    QgsOdbcPreparedStatement stmt = mConnection.prepareStatement( sql );
    QgsOdbcResultSetMetadata rsmd = stmt.getMetadata();
    for ( unsigned short i = 1; i <= rsmd.getColumnCount(); ++i )
    {
      QString baseTableName = rsmd.getBaseTableName( i );
      QString baseColumnName = rsmd.getBaseColumnName( i );
      QString schema = rsmd.getSchemaName( i );
      if ( schema.isEmpty() )
        schema = schemaName;

      AttributeField field;
      field.schemaName = schema;
      field.tableName = rsmd.getTableName( i );
      field.name = rsmd.getColumnName( i );
      field.typeName = rsmd.getColumnTypeName( i );
      field.type = rsmd.getColumnType( i );
      field.isSigned = rsmd.isSigned( i );
      field.isNullable = rsmd.isNullable( i );
      field.isAutoIncrement = rsmd.isAutoIncrement( i );
      field.size = static_cast<int>( rsmd.getColumnLength( i ) );
      field.precision = static_cast<int>( rsmd.getScale( i ) );

      if ( !schema.isEmpty() )
      {
        field.isUnique = isColumnUnique( schema, baseTableName, baseColumnName );
        // As field comments cannot be retrieved via ODBC, we get it from SYS.TABLE_COLUMNS.
        field.comment = getColumnComments( schema, baseTableName, baseColumnName );
        // We skip determining srid, as query layers don't use it.
      }

      callback( field );
    }
  }
  catch ( const QgsOdbcException &ex )
  {
    throw QgsHanaException( ex.what() );
  }
}

void QgsHanaConnection::readTableFields( const QString &schemaName, const QString &tableName, const function<void( const AttributeField &field )> &callback )
{
  QMap<QString, QMap<QString, bool>> clmAutoIncrement;
  auto isColumnAutoIncrement = [&]( const QString & columnName )
  {
    const QString key = QStringLiteral( "%1.%2" ).arg( schemaName, tableName );
    if ( !clmAutoIncrement.contains( key ) )
    {
      QString sql = QStringLiteral( "SELECT * FROM %1.%2" )
                    .arg( QgsHanaUtils::quotedIdentifier( schemaName ), QgsHanaUtils::quotedIdentifier( tableName ) );
      QgsOdbcPreparedStatement stmt = prepareStatement( sql );
      QgsOdbcResultSetMetadata rsmd = stmt.getMetadata();
      for ( unsigned short i = 1; i <= rsmd.getColumnCount(); ++i )
      {
        QString name = rsmd.getColumnName( i );
        bool isAutoIncrement = rsmd.isAutoIncrement( i );
        clmAutoIncrement[key].insert( name, isAutoIncrement );
      }
    }
    return clmAutoIncrement[key].value( columnName, false );
  };

  QMap<QString, QMap<QString, bool>> clmUniqueness;
  auto isColumnUnique = [&]( const QString & columnName )
  {
    const QString key = QStringLiteral( "%1.%2" ).arg( schemaName, tableName );
    if ( !clmUniqueness.contains( key ) )
      clmUniqueness.insert( key, getColumnsUniqueness( mConnection, schemaName, tableName ) );
    return clmUniqueness[key].value( columnName, false );
  };

  try
  {
    QgsOdbcResultSet rsColumns = getColumns( schemaName, tableName, QStringLiteral( "%" ) );
    while ( rsColumns.next() )
    {
      AttributeField field;
      field.schemaName = rsColumns.getString( 2/*TABLE_SCHEM*/ );
      field.tableName = rsColumns.getString( 3/*TABLE_NAME*/ );
      field.name = rsColumns.getString( 4/*COLUMN_NAME*/ );
      field.type = *rsColumns.getShort( 5/*DATA_TYPE*/ );
      field.typeName =  rsColumns.getString( 6/*TYPE_NAME*/ );
      if ( field.type == QgsOdbcDataTypes::Unknown )
        throw QgsHanaException( QString( "Type of the column '%1' is unknown" ).arg( field.name ) );
      field.size = *rsColumns.getInt( 7/*COLUMN_SIZE*/ );
      field.precision = static_cast<int>( *rsColumns.getShort( 9/*DECIMAL_DIGITS*/ ) );
      field.isSigned = field.type == QgsOdbcDataTypes::SmallInt || field.type == QgsOdbcDataTypes::Integer ||
                       field.type == QgsOdbcDataTypes::BigInt || field.type == QgsOdbcDataTypes::Decimal ||
                       field.type == QgsOdbcDataTypes::Numeric || field.type == QgsOdbcDataTypes::Real ||
                       field.type == QgsOdbcDataTypes::Float || field.type == QgsOdbcDataTypes::Double;
      QString isNullable = rsColumns.getString( 18/*IS_NULLABLE*/ );
      field.isNullable = ( isNullable == QLatin1String( "YES" ) || isNullable == QLatin1String( "TRUE" ) );
      field.isAutoIncrement = isColumnAutoIncrement( field.name );
      field.isUnique = isColumnUnique( field.name );
      if ( field.isGeometry() )
        field.srid = getColumnSrid( schemaName, tableName, field.name );
      field.comment = rsColumns.getString( 12/*REMARKS*/ );

      callback( field );
    }
  }
  catch ( const QgsOdbcException &ex )
  {
    throw QgsHanaException( ex.what() );
  }
}

QVector<QgsHanaSchemaProperty> QgsHanaConnection::getSchemas( const QString &ownerName )
{
  QString sql = QStringLiteral( "SELECT SCHEMA_NAME, SCHEMA_OWNER FROM SYS.SCHEMAS WHERE "
                                "HAS_PRIVILEGES = 'TRUE' AND %1 AND "
                                "SCHEMA_NAME NOT LIKE_REGEXPR 'SYS|_SYS.*|UIS|SAP_XS|SAP_REST|HANA_XS|XSSQLCC_'" )
                .arg( !ownerName.isEmpty() ? QStringLiteral( "SCHEMA_OWNER = ?" ) : QStringLiteral( "SCHEMA_OWNER IS NOT NULL" ) );

  QVector<QgsHanaSchemaProperty> list;

  try
  {
    QgsOdbcPreparedStatement stmt = mConnection.prepareStatement( sql );
    if ( !ownerName.isEmpty() )
      stmt.setNString( 1, ownerName );
    QgsOdbcResultSet rsSchemas = stmt.executeQuery();
    while ( rsSchemas.next() )
    {
      list.push_back( {rsSchemas.getString( 1 ), rsSchemas.getString( 2 )} );
    }
    rsSchemas.close();
  }
  catch ( const QgsOdbcException &ex )
  {
    throw QgsHanaException( ex.what() );
  }

  return list;
}

QStringList QgsHanaConnection::getLayerPrimaryKey( const QString &schemaName, const QString &tableName )
{
  try
  {
    QgsOdbcDatabaseMetadata dbmd = mConnection.getDatabaseMetadata();
    QgsOdbcResultSet rsPrimaryKeys = dbmd.getPrimaryKeys( QString(),
                                     schemaName,
                                     tableName );
    QMap<int, QString> pos2Name;
    while ( rsPrimaryKeys.next() )
    {
      QString clmName = rsPrimaryKeys.getString( 4 /*COLUMN_NAME*/ );
      int pos = *rsPrimaryKeys.getInt( 5 /*KEY_SEQ*/ );
      pos2Name.insert( pos, clmName );
    }
    rsPrimaryKeys.close();
    return pos2Name.values();
  }
  catch ( const QgsOdbcException &ex )
  {
    throw QgsHanaException( ex.what() );
  }
}

QStringList QgsHanaConnection::getPrimaryKeyCandidates( const QgsHanaLayerProperty &layerProperty )
{
  if ( !layerProperty.isView )
    return QStringList();

  QStringList ret;
  QgsOdbcResultSet rsColumns = getColumns( layerProperty.schemaName, layerProperty.tableName, QStringLiteral( "%" ) );
  while ( rsColumns.next() )
  {
    int dataType = *rsColumns.getInt( 5/*DATA_TYPE */ );
    // We exclude GEOMETRY and LOB columns
    if ( dataType == 29812 /* GEOMETRY TYPE */ || dataType == QgsOdbcDataTypes::LongVarBinary ||
         dataType == QgsOdbcDataTypes::LongVarChar || dataType == QgsOdbcDataTypes::WLongVarChar )
      continue;
    ret << rsColumns.getString( 4/*COLUMN_NAME */ );
  }
  rsColumns.close();
  return ret;
}

Qgis::WkbType QgsHanaConnection::getColumnGeometryType( const QString &querySource, const QString &columnName )
{
  if ( columnName.isEmpty() )
    return Qgis::WkbType::NoGeometry;

  Qgis::WkbType ret = Qgis::WkbType::Unknown;
  QString sql = QStringLiteral( "SELECT upper(%1.ST_GeometryType()), %1.ST_Is3D(), %1.ST_IsMeasured() FROM %2 "
                                "WHERE %1 IS NOT NULL LIMIT %3" ).arg(
                  QgsHanaUtils::quotedIdentifier( columnName ),
                  querySource,
                  QString::number( GEOMETRIES_SELECT_LIMIT ) );

  try
  {
    QgsOdbcStatement stmt = mConnection.createStatement();
    QgsOdbcResultSet rsGeomInfo = stmt.executeQuery( sql );
    while ( rsGeomInfo.next() )
    {
      QString type = rsGeomInfo.getString( 1 );
      QgsOdbcNullable<int32_t> hasZ = rsGeomInfo.getInt( 2 );
      QgsOdbcNullable<int32_t> hasM = rsGeomInfo.getInt( 3 );
      bool hasZValue = hasZ.isNull() ? false : *hasZ == 1;
      bool hasMValue = hasM.isNull() ? false : *hasM == 1;
      Qgis::WkbType geomType = QgsWkbTypes::singleType( QgsHanaUtils::toWkbType( type, hasZValue, hasMValue ) );
      if ( geomType == Qgis::WkbType::Unknown )
        continue;
      if ( ret == Qgis::WkbType::Unknown )
        ret = geomType;
      else if ( ret != geomType )
      {
        ret = Qgis::WkbType::Unknown;
        break;
      }
    }
    rsGeomInfo.close();
  }
  catch ( const QgsOdbcException &ex )
  {
    throw QgsHanaException( ex.what() );
  }

  return ret;
}

Qgis::WkbType QgsHanaConnection::getColumnGeometryType( const QString &schemaName, const QString &tableName, const QString &columnName )
{
  QString querySource = QStringLiteral( "%1.%2" ).arg( QgsHanaUtils::quotedIdentifier( schemaName ),
                        QgsHanaUtils::quotedIdentifier( tableName ) );
  return getColumnGeometryType( querySource, columnName );
}

QString QgsHanaConnection::getColumnDataType( const QString &schemaName, const QString &tableName, const QString &columnName )
{
  const char *sql = "SELECT DATA_TYPE_NAME FROM SYS.TABLE_COLUMNS WHERE SCHEMA_NAME = ? AND "
                    "TABLE_NAME = ? AND COLUMN_NAME = ?";

  QString ret;
  try
  {
    QgsOdbcPreparedStatement stmt = mConnection.prepareStatement( sql );
    stmt.setNString( 1, schemaName );
    stmt.setNString( 2, tableName );
    stmt.setNString( 3, columnName );
    QgsOdbcResultSet rsDataType = stmt.executeQuery();
    while ( rsDataType.next() )
    {
      ret = rsDataType.getString( 1 );
    }
    rsDataType.close();
  }
  catch ( const QgsOdbcException &ex )
  {
    throw QgsHanaException( ex.what() );
  }

  return ret;
}

int QgsHanaConnection::getColumnSrid( const QString &schemaName, const QString &tableName, const QString &columnName )
{
  if ( columnName.isEmpty() )
    return -1;

  try
  {
    QString sql = QStringLiteral( "SELECT SRS_ID FROM SYS.ST_GEOMETRY_COLUMNS "
                                  "WHERE SCHEMA_NAME = ? AND TABLE_NAME = ? AND COLUMN_NAME = ?" );
    QgsOdbcPreparedStatement stmt = mConnection.prepareStatement( sql );
    stmt.setNString( 1, schemaName );
    stmt.setNString( 2, tableName );
    stmt.setNString( 3, columnName );
    QgsOdbcResultSet rsSrid = stmt.executeQuery();
    int srid = getSrid( rsSrid );

    if ( srid != -1 )
      return srid;

    sql = QStringLiteral( "SELECT %1.ST_SRID() FROM %2.%3 WHERE %1 IS NOT NULL LIMIT %4" )
          .arg( QgsHanaUtils::quotedIdentifier( columnName ),
                QgsHanaUtils::quotedIdentifier( schemaName ),
                QgsHanaUtils::quotedIdentifier( tableName ),
                QString::number( GEOMETRIES_SELECT_LIMIT ) );
    QgsOdbcStatement stmt2 = mConnection.createStatement();
    QgsOdbcResultSet rsSrid2 = stmt2.executeQuery( sql );
    return getSrid( rsSrid2 );
  }
  catch ( const QgsOdbcException &ex )
  {
    throw QgsHanaException( ex.what() );
  }
}

int QgsHanaConnection::getColumnSrid( const QString &sql, const QString &columnName )
{
  if ( columnName.isEmpty() )
    return -1;

  QString query = QStringLiteral( "SELECT %1.ST_SRID() FROM (%2) WHERE %1 IS NOT NULL LIMIT %3" )
                  .arg( QgsHanaUtils::quotedIdentifier( columnName ),
                        sql,
                        QString::number( GEOMETRIES_SELECT_LIMIT ) );

  try
  {
    QgsOdbcPreparedStatement stmt = mConnection.prepareStatement( query );
    QgsOdbcResultSet rsSrid = stmt.executeQuery();
    return getSrid( rsSrid );
  }
  catch ( const QgsOdbcException &ex )
  {
    throw QgsHanaException( ex.what() );
  }
}

QgsOdbcResultSet QgsHanaConnection::getColumns( const QString &schemaName, const QString &tableName, const QString &fieldName )
{
  try
  {
    QgsOdbcDatabaseMetadata metadata = mConnection.getDatabaseMetadata();
    return QgsOdbcResultSet( metadata.getColumns( QString(), schemaName, tableName, fieldName ) );
  }
  catch ( const QgsOdbcException &ex )
  {
    throw QgsHanaException( ex.what() );
  }
}

bool  QgsHanaConnection::isTable( const QString &schemaName, const QString &tableName )
{
  QString sql = QStringLiteral( "SELECT COUNT(*) FROM SYS.TABLES WHERE SCHEMA_NAME = ? AND TABLE_NAME = ?" );
  return executeCountQuery( sql, {schemaName, tableName } ) == 1;
}
