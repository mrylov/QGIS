#include "qgsodbcresultset.h"
#include "qgsodbcstatement.h"
#include <QString>

using namespace NS_ODBC;

namespace
{
  const char16_t *toUtf16( const QString &str )
  {
    return reinterpret_cast<const char16_t *>( str.utf16() );
  }
}

QgsOdbcStatement::QgsOdbcStatement( StatementRef &&stmt )
  : mStatement( std::move(stmt) )
{
}

void QgsOdbcStatement::execute( const QString &sql )
{
  mStatement->execute( toUtf16( sql ) );
}

QgsOdbcResultSet QgsOdbcStatement::executeQuery( const QString &sql )
{
  return QgsOdbcResultSet( mStatement->executeQuery( toUtf16( sql ) ) );
}
