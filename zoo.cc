/*

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org
 1999-2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <iostream.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <string.h>

// KDE includes
#include <kurl.h>
#include <qstringlist.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

// ark includes
#include "zoo.h"

// the generic viewer to which to send header and column info.
#include "viewer.h"

QString fixYear(const char *strYear);
QString fixTime(const QString &_strTime);

ZooArch::ZooArch( ArkSettings *_settings, Viewer *_gui,
		  const QString & _fileName )
  : Arch(_settings, _gui, _fileName )
{
  kDebugInfo(1601, "ZooArch constructor");
  _settings->readZooProperties();
  m_archiver_program = "zoo";
}

void ZooArch::processLine( char *_line )
{
  char columns[11][80];
  char filename[4096];

  // Note: I'm reversing the ratio and the length for better display

  sscanf(_line, " %[0-9] %[0-9%] %[0-9] %[0-9] %[a-zA-Z] %[0-9]%[ ]%11[ 0-9:+-]%2[C ]%[^\n]",
	 columns[1], columns[0], columns[2], columns[3], columns[7],
	 columns[8], columns[9], columns[4], columns[10], filename);


  kDebugInfo(1601, "The actual file is %s", (const char *)filename);

  QString year = fixYear(columns[8]);

  kDebugInfo(1601, "The year is %s", (const char *)year);

  QString strDate;
  strDate.sprintf("%s-%.2d-%.2d", (const char *)year,
		    getMonth(columns[7]), atoi(columns[3]));

  strcpy(columns[3], (const char *)strDate);
  kDebugInfo(1601, "New timestamp is %s", columns[3]);

  strcat(columns[3], " ");
  strcat(columns[3], (const char *)fixTime(columns[4]));

  QStringList list;
  list.append(QString::fromLocal8Bit(filename));
  for (int i=0; i<4; i++)
    {
      list.append(QString::fromLocal8Bit(columns[i]));
    }
  m_gui->add(&list); // send to GUI
}

void ZooArch::open()
{
  kDebugInfo( 1601, "+ZooArch::open");
  setHeaders();

  m_buffer[0] = '\0';
  m_header_removed = false;
  m_finished = false;


  KProcess *kp = new KProcess;
  *kp << m_archiver_program << "l" << m_filename.local8Bit();
  connect( kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotReceivedTOC(KProcess*, char*, int)));
  connect( kp, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(slotReceivedOutput(KProcess*, char*, int)));

  connect( kp, SIGNAL(processExited(KProcess*)), this,
	   SLOT(slotOpenExited(KProcess*)));

  if (kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false)
    {
      KMessageBox::error( 0, i18n("Couldn't start a subprocess.") );
      emit sigOpen(this, false, QString::null, 0 );
    }

  kDebugInfo( 1601, "-ZooArch::open");
}

void ZooArch::setHeaders()
{
  kDebugInfo( 1601, "+ZooArch::setHeaders");
  QStringList list;
  list.append(i18n(" Filename "));
  list.append(i18n(" Ratio "));
  list.append(i18n(" Length "));
  list.append(i18n(" Size Now "));
  list.append(i18n(" Timestamp "));

  // which columns to align right
  int *alignRightCols = new int[2];
  alignRightCols[0] = 2;
  alignRightCols[1] = 3;

  m_gui->setHeaders(&list, alignRightCols, 2);
  delete [] alignRightCols;

  kDebugInfo( 1601, "-ZooArch::setHeaders");
}


void ZooArch::slotReceivedTOC(KProcess*, char* _data, int _length)
{
  kDebugInfo(1601, "+ZooArch::slotReceivedTOC");
  char c = _data[_length];
  _data[_length] = '\0';
	
  m_settings->appendShellOutputData( _data );

  char line[1024] = "";
  char *tmpl = line;

  char *tmpb;

  for( tmpb = m_buffer; *tmpb != '\0'; tmpl++, tmpb++ )
    *tmpl = *tmpb;

  for( tmpb = _data; *tmpb != '\n'; tmpl++, tmpb++ )
    *tmpl = *tmpb;
		
  tmpb++;
  *tmpl = '\0';

  if( *tmpb == '\0' )
    m_buffer[0]='\0';

  if( !strstr( line, "----" ) )
    {
      if( m_header_removed && !m_finished ){
	processLine( line );
      }
    }
  else if(!m_header_removed)
    m_header_removed = true;
  else
    m_finished = true;

  bool stop = (*tmpb == '\0');

  while( !stop && !m_finished )
    {
      tmpl = line; *tmpl = '\0';

      for(; (*tmpb!='\n') && (*tmpb!='\0'); tmpl++, tmpb++)
	*tmpl = *tmpb;

      if( *tmpb == '\n' )
	{
	  *tmpl = '\n';
	  tmpl++;
	  *tmpl = '\0';
	  tmpb++;

	if( !strstr( line, "----" ) )
	  {
	    if( m_header_removed ){
	      processLine( line );
	    }
	  }
	else if( !m_header_removed )
	  m_header_removed = true;
	else
	  {
	    m_finished = true;
	  }
	}
      else if (*tmpb == '\0' )
	{
	  *tmpl = '\0';
	  strcpy( m_buffer, line );
	  stop = true;
	}
    }
  
  _data[_length] = c;
  kDebugInfo(1601, "-ZooArch::slotReceivedTOC");
}

void ZooArch::create()
{
  emit sigCreate(this, true, m_filename,
		 Arch::Extract | Arch::Delete | Arch::Add 
		 | Arch::View);
}

void ZooArch::addDir(const QString & _dirName)
{
  if (! _dirName.isEmpty())
  {
    QStringList list;
    list.append(_dirName);
    addFile(&list);
  }
}

void ZooArch::addFile( QStringList *urls )
{
  kDebugInfo( 1601, "+ZooArch::addFile");
  KProcess *kp = new KProcess;
  kp->clearArguments();
  *kp << m_archiver_program;
	
  if (m_settings->getReplaceOnlyNew() )
    *kp << "-update";
  else
    *kp << "-add";

  *kp << m_filename.local8Bit() ;

  QString base;
  QString url;
  QString file;

	
  QStringList::ConstIterator iter;
  for (iter = urls->begin(); iter != urls->end(); ++iter )
  {
    url = *iter;
    // comment out for now until I figure out what happened to this function!
    //    KURL::decodeURL(url); // Because of special characters
    file = url.right(url.length()-5);

    if( file[file.length()-1]=='/' )
      file[file.length()-1]='\0';
    if( ! m_settings->getaddPath() )
    {
      int pos;
      pos = file.findRev( '/' );
      base = file.left( pos );
      pos++;
      chdir( base );
      base = file.right( file.length()-pos );
      file = base;
    }
    *kp << file;
  }
  connect( kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotReceivedOutput(KProcess*, char*, int)));
  connect( kp, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(slotReceivedOutput(KProcess*, char*, int)));

  connect( kp, SIGNAL(processExited(KProcess*)), this,
	   SLOT(slotAddExited(KProcess*)));

  if (kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false)
    {
      KMessageBox::error( 0, i18n("Couldn't start a subprocess.") );
      emit sigAdd(false);
    }

  kDebugInfo( 1601, "+ZooArch::addFile");
}

void ZooArch::unarchFile(QStringList *_fileList, const QString & _destDir)
{
  // if _fileList is empty, we extract all.
  // if _destDir is empty, look at settings for extract directory

  kDebugInfo( 1601, "+ZooArch::unarchFile");
  QString dest;

  if (_destDir.isEmpty() || _destDir.isNull())
    dest = m_settings->getExtractDir();
  else dest = _destDir;

  // zoo has no option to specify the destination directory
  // so I have to change to it.

  int ret = chdir((const char *)dest);
 // I already checked the validity of the dir before coming here
  ASSERT(ret == 0); 


  QString tmp;
	
  KProcess *kp = new KProcess;
  kp->clearArguments();
  
  *kp << m_archiver_program;

  if (!m_settings->getZooOverwriteFiles())
    *kp << "x";
  else
    *kp << "xOOS";
  *kp << m_filename;
  
  // if the list is empty, no filenames go on the command line,
  // and we then extract everything in the archive.
  if (_fileList)
    {
      for ( QStringList::Iterator it = _fileList->begin();
	    it != _fileList->end(); ++it ) 
	{
	  *kp << (*it).latin1() ;
	}
    }
 
  connect( kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotReceivedOutput(KProcess*, char*, int)));
  connect( kp, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(slotReceivedOutput(KProcess*, char*, int)));

  connect( kp, SIGNAL(processExited(KProcess*)), this,
	   SLOT(slotExtractExited(KProcess*)));
  
  if (kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false)
    {
      KMessageBox::error( 0, i18n("Couldn't start a subprocess.") );
      emit sigExtract(false);
    }
}

void ZooArch::remove(QStringList *list)
{
  kDebugInfo( 1601, "+ZooArch::remove");

  if (!list)
    return;

  m_shellErrorData = "";
  KProcess *kp = new KProcess;
  kp->clearArguments();
  
  *kp << m_archiver_program << "D" << m_filename.local8Bit();
  for ( QStringList::Iterator it = list->begin();
	it != list->end(); ++it )
    {
      QString str = *it;
      *kp << str.local8Bit();
    }

  connect( kp, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotReceivedOutput(KProcess*, char*, int)));
  connect( kp, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(slotReceivedOutput(KProcess*, char*, int)));

  connect( kp, SIGNAL(processExited(KProcess*)), this,
	   SLOT(slotDeleteExited(KProcess*)));

  if (kp->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false)
    {
      KMessageBox::error( 0, i18n("Couldn't start a subprocess.") );
      emit sigDelete(false);
    }
  
  kDebugInfo( 1601, "-ZooArch::remove");
}


QString fixYear(const char *strYear)
{
  // returns 4-digit year by guessing from two-char year string.
  // Remember: this is used for file timestamps. There probably aren't any
  // files that were created before 1970, so that's our cutoff. Of course,
  // in 2070 we'll have some problems....

  char fourDigits[5] = {0,0,0,0,0};
  if (atoi(strYear) > 70)
    {
      strcpy(fourDigits, "19");
    }
  else
    {
      strcpy(fourDigits, "20");
    }
  strcat(fourDigits, strYear);
  return fourDigits;
}

QString fixTime(const QString &_strTime)
{
  // it may have come from a different time zone... get rid of trailing
  // +3 or -3 etc. 
  QString strTime = _strTime;

  if (strTime.contains("+") || strTime.contains("-"))
    {
      QCharRef c = strTime.at(8);
      int offset = atoi(strTime.right(strTime.length() - 9));
      kDebugInfo(1601, "Offset is %d\n", offset);
      QString strHour = strTime.left(2);
      int nHour = atoi(strHour);
      if (c == '+' || c == '-')
	{
	  if (c == '+')
	    nHour = (nHour + offset) % 24;
	  else if (c == '-')
	    {
	      nHour -= offset;
	      if (nHour < 0)
		nHour += 24;
	    }
	  strTime = strTime.left(8);
	  strTime.sprintf("%2.2d%s", nHour, (const char *)strTime.right(6));
	  kDebugInfo(1601, "The new time is %s\n", (const char *) strTime);
	}
    }
  return strTime;
}

#include "zoo.moc"
