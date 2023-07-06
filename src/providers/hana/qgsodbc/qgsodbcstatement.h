/***************************************************************************
   qgsodbcstatement.h
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
#ifndef QGSODBCSTATEMENT_H
#define QGSODBCSTATEMENT_H

#include <qglobal.h>
#include "odbc/Statement.h"

class QString;
class QgsOdbcResultSet;

class QgsOdbcStatement
{
  private:
    friend class QgsOdbcConnection;

  private:
    QgsOdbcStatement( NS_ODBC::StatementRef &&stmt );

  protected:
    Q_DISABLE_COPY( QgsOdbcStatement )

  public:
    void execute( const QString &sql );
    QgsOdbcResultSet executeQuery( const QString &sql );

  private:
    NS_ODBC::StatementRef mStatement;
};

#endif // QGSODBCSTATEMENT_H
