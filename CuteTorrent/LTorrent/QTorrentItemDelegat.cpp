#include "QTorrentItemDelegat.h"



/*
 * This file Copyright (C) Mnemosyne LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 * $Id: torrent-delegate.cc 13244 2012-03-04 13:15:43Z jordan $
 */

#include <iostream>

#include <QApplication>
#include <QFont>
#include <QFontMetrics>
#include <QIcon>
#include <QModelIndex>
#include <QPainter>
#include <QPixmap>
#include <QPixmapCache>
#include <QStyleOptionProgressBarV2>
#include <QMessageBox>
#include "QTorrentDisplayModel.h"
#include "Torrent.h"



enum
{
   GUI_PAD = 6,
   BAR_HEIGHT = 12
};


QTorrentItemDelegat::QTorrentItemDelegat(const QTorrentItemDelegat &dlg): QStyledItemDelegate(0) ,myProgressBarStyle(new QStyleOptionProgressBarV2)
{
	myProgressBarStyle->minimum = 0;
    myProgressBarStyle->maximum = 100;
	greenBrush=dlg.greenBrush;
	greenBack=dlg.greenBack;
	blueBrush=dlg.blueBrush;
	blueBack=dlg.blueBack;
}

QTorrentItemDelegat :: QTorrentItemDelegat(): QStyledItemDelegate(0) ,myProgressBarStyle(new QStyleOptionProgressBarV2){}

QColor QTorrentItemDelegat::greenBrush = QColor("forestgreen");
QColor QTorrentItemDelegat ::greenBack = QColor("darkseagreen");

QColor QTorrentItemDelegat ::blueBrush = QColor("steelblue");
QColor QTorrentItemDelegat ::blueBack = QColor("lightgrey");
QTorrentItemDelegat :: QTorrentItemDelegat( QObject * parent ):
    QStyledItemDelegate( parent ),
    myProgressBarStyle( new QStyleOptionProgressBarV2 )
{
	
    myProgressBarStyle->minimum = 0;
    myProgressBarStyle->maximum = 100;

    
}

QTorrentItemDelegat :: ~QTorrentItemDelegat( )
{
    delete myProgressBarStyle;
}



QSize QTorrentItemDelegat :: margin( const QStyle& style ) const
{
    Q_UNUSED( style );

    return QSize( 4, 4 );
}




namespace
{
    int MAX3( int a, int b, int c )
    {
        const int ab( a > b ? a : b );
        return ab > c ? ab : c;
    }
}

QSize
QTorrentItemDelegat :: sizeHint( const QStyleOptionViewItem& option, const Torrent& tor ) const
{

    const QStyle* style( QApplication::style( ) );
     const int iconSize( style->pixelMetric( QStyle::PM_MessageBoxIconSize ) );

    QFont nameFont( option.font );
	nameFont.setWeight( QFont::Bold );
    const QFontMetrics nameFM( nameFont );
	
	const QString nameStr( tor.GetName() );
	
const int nameWidth = nameFM.width( nameStr );
	
    QFont statusFont( option.font );
    statusFont.setPointSize( int( option.font.pointSize( ) * 0.9 ) );
    const QFontMetrics statusFM( statusFont );
	const QString statusStr( GetStatusString(tor) );
    const int statusWidth = statusFM.width( statusStr );
    QFont progressFont( statusFont );
    const QFontMetrics progressFM( progressFont );
    const QString progressStr( tor.GetProgresString() );
    const int progressWidth = progressFM.width( progressStr );
    const QSize m( margin( *style ) );
	
    return QSize( m.width()*2 + iconSize + GUI_PAD + MAX3( nameWidth, statusWidth, progressWidth ),
                  m.height()*3 + nameFM.lineSpacing() + statusFM.lineSpacing() + BAR_HEIGHT + progressFM.lineSpacing() );
}

QSize
QTorrentItemDelegat :: sizeHint( const QStyleOptionViewItem  & option,
                             const QModelIndex           & index ) const
{
	Torrent*  tor( index.data( QTorrentDisplayModel::TorrentRole ).value<Torrent*>() );
    return sizeHint( option, *tor );
}

void
QTorrentItemDelegat :: paint( QPainter                    * painter,
                          const QStyleOptionViewItem  & option,
                          const QModelIndex           & index) const
{

    Torrent*  tor( index.data( QTorrentDisplayModel::TorrentRole ).value<Torrent*>() );
    painter->save( );
    painter->setClipRect( option.rect );
    drawTorrent( painter, option, *tor, index.row());
    painter->restore( );
}

void
QTorrentItemDelegat :: setProgressBarPercentDone( const QStyleOptionViewItem& option, const Torrent& tor ) const
{

      
	const int scaledProgress = tor.GetProgress();
	 myProgressBarStyle->direction = option.direction;
    myProgressBarStyle->progress =scaledProgress;
   
}
QString QTorrentItemDelegat ::GetProgressString(const Torrent& tor) const
{
	if (tor.isDownloading())
	{
		return QString::fromLocal8Bit("%1 Загружено %2 из %3").arg(tor.GetProgresString()).arg(tor.GetTotalDownloaded()).arg(tor.GetTotalSize());
	}
	if (tor.isSeeding())
	{
		return QString::fromLocal8Bit("%1 - %3 Розданно %2").arg(tor.GetProgresString()).arg(tor.GetTotalUploaded()).arg(tor.GetTotalSize());
	}
	return tor.GetProgresString();
}
QString QTorrentItemDelegat ::GetStatusString(const Torrent& tor) const
{
	QString upSpeed(tor.GetUploadSpeed());
	QString downSpeed(tor.GetDwonloadSpeed());
	QString status(tor.GetStatusString());
	bool hasError(tor.hasError());
	if (hasError)
	{
		return tor.GetErrorMessage();
	}
	if (tor.isPaused())
	{
		return QString::fromLocal8Bit("Приостановлен");
	}
	if (tor.isDownloading())
	{
		return QString::fromLocal8Bit("%1: %2 %3 - %4 %5").arg(status).arg(QChar(0x2193)).arg(downSpeed).arg(QChar(0x2191)).arg(upSpeed);
	}
	if (tor.isSeeding())
	{
		if (!upSpeed.isEmpty())
			return QString::fromLocal8Bit("%1 - %2 %3").arg(status).arg(QChar(0x2191)).arg(upSpeed);
	}
	return status;
}
void
QTorrentItemDelegat :: drawTorrent( QPainter * painter, const QStyleOptionViewItem& option, const Torrent& tor, int row ) const
{
//	MessageBoxA(0,"drawTorrent","drawTorrent",0);
    const QStyle * style( QApplication::style( ) );
     const int iconSize( style->pixelMetric( QStyle::PM_LargeIconSize ) );
    QFont nameFont( option.font );
    nameFont.setWeight( QFont::Bold );
    const QFontMetrics nameFM( nameFont );

	
	const QString nameStr( tor.GetName() );
	const QSize nameSize( nameFM.size( 0, nameStr ) );
    QFont statusFont( option.font );
    statusFont.setPointSize( int( option.font.pointSize( ) * 0.9 ) );
    const QFontMetrics statusFM( statusFont );
    const QString statusStr( GetStatusString(tor));
    QFont progressFont( statusFont );
	
    const QFontMetrics progressFM( progressFont );
	const QString progressStr( GetProgressString(tor) );
	QFont sizeFont( statusFont );
	bool isPaused(tor.isPaused());

	
    painter->save( );
	if (row & 1) 
	{
		painter->fillRect(option.rect, QColor(220, 220, 220));
	}
    if (option.state & QStyle::State_Selected) {
        QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
                                  ? QPalette::Normal : QPalette::Disabled;
        if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
            cg = QPalette::Inactive;

        painter->fillRect(option.rect, option.palette.brush(cg, QPalette::Highlight));
    }
	
    QIcon::Mode im;
    if( isPaused || !(option.state & QStyle::State_Enabled ) ) im = QIcon::Disabled;
    else if( option.state & QStyle::State_Selected ) im = QIcon::Selected;
    else im = QIcon::Normal;

    QIcon::State qs;
    if( isPaused ) qs = QIcon::Off;
    else qs = QIcon::On;

    QPalette::ColorGroup cg = QPalette::Normal;
    if( isPaused || !(option.state & QStyle::State_Enabled ) ) cg = QPalette::Disabled;
    if( cg == QPalette::Normal && !(option.state & QStyle::State_Active ) ) cg = QPalette::Inactive;

    QPalette::ColorRole cr;
    if( option.state & QStyle::State_Selected ) cr = QPalette::HighlightedText;
    else cr = QPalette::Text;

    QStyle::State progressBarState( option.state );
    if( isPaused ) progressBarState = QStyle::State_None;
    progressBarState |= QStyle::State_Small;

    // layout
    const QSize m( margin( *style ) );
    QRect fillArea( option.rect );
    fillArea.adjust( m.width(), m.height(), -m.width(), -m.height() );
    QRect iconArea( fillArea.x( ), fillArea.y( ) + ( fillArea.height( ) - iconSize ) / 2, iconSize, iconSize );
    QRect nameArea( iconArea.x( ) + iconArea.width( ) + GUI_PAD, fillArea.y( ),
                    fillArea.width( ) - GUI_PAD - iconArea.width( ), nameSize.height( ) );
    QRect statusArea( nameArea );
	
    statusArea.moveTop( nameArea.y( ) + nameFM.lineSpacing( ) );
    statusArea.setHeight( nameSize.height( ) );
	

    QRect barArea( statusArea );
	
    barArea.setHeight( BAR_HEIGHT );
    barArea.moveTop( statusArea.y( ) + statusFM.lineSpacing( ) );
    QRect progArea( statusArea );
    progArea.moveTop( barArea.y( ) + barArea.height( ) );
	

    // render
    if( tor.hasError( ) )
        painter->setPen( QColor( "red" ) );
    else
        painter->setPen( option.palette.color( cg, cr ) );
	tor.GetMimeTypeIcon().paint( painter, iconArea, Qt::AlignCenter, im, qs );
    painter->setFont( nameFont );
    painter->drawText( nameArea, 0, nameFM.elidedText( nameStr, Qt::ElideRight, nameArea.width( ) ) );
 	painter->setFont( statusFont );
    painter->drawText( statusArea, 0, statusFM.elidedText( statusStr, Qt::ElideRight, statusArea.width( ) ) );
    painter->setFont( progressFont );
    painter->drawText( progArea, 0, progressFM.elidedText( progressStr, Qt::ElideRight, progArea.width( ) ) );
    myProgressBarStyle->rect = barArea;
	if ( tor.isDownloading() ) {
        myProgressBarStyle->palette.setBrush( QPalette::Highlight, blueBrush );
        myProgressBarStyle->palette.setColor( QPalette::Base, blueBack );
        myProgressBarStyle->palette.setColor( QPalette::Background, blueBack );
    }
    else if ( tor.isSeeding() ) {
        myProgressBarStyle->palette.setBrush( QPalette::Highlight, greenBrush );
        myProgressBarStyle->palette.setColor( QPalette::Base, greenBack );
        myProgressBarStyle->palette.setColor( QPalette::Background, greenBack );
    }
    myProgressBarStyle->state = progressBarState;
    setProgressBarPercentDone( option, tor );

    style->drawControl( QStyle::CE_ProgressBar, myProgressBarStyle, painter );

    painter->restore( );
}
