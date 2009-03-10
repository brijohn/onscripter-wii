/* -*- C++ -*-
 *
 *  NsaReader.cpp - Reader from a NSA archive
 *
 *  Copyright (c) 2001-2008 Ogapee. All rights reserved.
 *
 *  ogapee@aqua.dti2.ne.jp
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "NsaReader.h"
#include <cstdio>
#include <string.h>
#define NSA_ARCHIVE_NAME "arc"
#define NSA_ARCHIVE_NAME2 "arc%d"

NsaReader::NsaReader( DirPaths *path, const unsigned char *key_table )
        :SarReader( path, key_table )
{
    sar_flag = true;
    num_of_nsa_archives = 0;

    if (key_table)
        nsa_archive_ext = "___";
    else
        nsa_archive_ext = "nsa";
}

NsaReader::~NsaReader()
{
}

int NsaReader::open( const char *nsa_path, int archive_type )
{
    int i,j,n;
    FILE *fp;
    char archive_name[256], archive_name2[256];

    if ( !SarReader::open( "arc.sar" ) ) {
        sar_flag = true;
    }
    else {
        sar_flag = false;
    }

    if ( !nsa_path ) nsa_path = "";


    i = j = -1;
    n = 0;
    while ((i<MAX_EXTRA_ARCHIVE) && (n<archive_path->get_num_paths())) {
        if (j < 0) {
            sprintf( archive_name, "%s%s.%s", nsa_path, NSA_ARCHIVE_NAME, nsa_archive_ext );
            sprintf(archive_name2, "%s%s", archive_path->get_path(n), archive_name);
        } else {
            sprintf( archive_name2, NSA_ARCHIVE_NAME2, j+1 );
            sprintf( archive_name, "%s%s.%s", nsa_path, archive_name2, nsa_archive_ext );
            sprintf(archive_name2, "%s%s", archive_path->get_path(n), archive_name);
        }
        fp = std::fopen(archive_name2, "rb");
        if (fp != NULL) {
            if (i < 0) {
                archive_info_nsa.file_handle = fp;
                archive_info_nsa.file_name = new char[strlen(archive_name2)+1];
                strcpy(archive_info_nsa.file_name, archive_name2);
                readArchive( &archive_info_nsa, archive_type );
            } else {
                archive_info2[i].file_handle = fp;
                archive_info2[i].file_name = new char[strlen(archive_name2)+1];
                strcpy(archive_info2[i].file_name, archive_name2);
                readArchive( &archive_info2[i], archive_type );
            }
            i++;
            j++;
        } else {
            j = -1;
            n++;
        }
    }

    if (i < 0) {
        // didn't find any (main) archive files
        fprintf( stderr, "can't open archive file %s\n", archive_name );
        return -1;
    } else {
        num_of_nsa_archives = i+1;
        return 0;
    }
}

int NsaReader::openForConvert( const char *nsa_name, int archive_type )
{
    sar_flag = false;
    if ( ( archive_info_nsa.file_handle = ::fopen( nsa_name, "rb" ) ) == NULL ){
        fprintf( stderr, "can't open file %s\n", nsa_name );
        return -1;
    }

    readArchive( &archive_info_nsa, archive_type );

    return 0;
}

int NsaReader::writeHeader( FILE *fp, int archive_type )
{
    ArchiveInfo *ai = &archive_info;
    return writeHeaderSub( ai, fp, archive_type );
}

size_t NsaReader::putFile( FILE *fp, int no, size_t offset, size_t length, size_t original_length, int compression_type, bool modified_flag, unsigned char *buffer )
{
    ArchiveInfo *ai = &archive_info;
    return putFileSub( ai, fp, no, offset, length, original_length , compression_type, modified_flag, buffer );
}

const char *NsaReader::getArchiveName() const
{
    return "nsa";
}

int NsaReader::getNumFiles(){
    int i;
    int total = archive_info.num_of_files; // start with sar files, if any

    total += archive_info_nsa.num_of_files; // add in the arc.nsa files

    for ( i=0 ; i<num_of_nsa_archives ; i++ ) total += archive_info2[i].num_of_files; // add in the arc?.nsa files
    
    return total;
}

size_t NsaReader::getFileLengthSub( ArchiveInfo *ai, const char *file_name )
{
    unsigned int i = getIndexFromFile( ai, file_name );

    if ( i == ai->num_of_files ) return 0;

    if ( ai->fi_list[i].compression_type == NO_COMPRESSION ){
        int type = getRegisteredCompressionType( file_name );
        if ( type == NBZ_COMPRESSION || type == SPB_COMPRESSION )
            return getDecompressedFileLength( type, ai->file_handle, ai->fi_list[i].offset );
    }
    
    return ai->fi_list[i].original_length;
}

size_t NsaReader::getFileLength( const char *file_name )
{
    size_t ret;
    int i;
    
    // direct read
    if ( ( ret = DirectReader::getFileLength( file_name ) ) ) return ret;
    
    // nsa read
    if ( ( ret = getFileLengthSub( &archive_info_nsa, file_name )) ) return ret;

    // nsa? read
    for ( i=0 ; i<num_of_nsa_archives-1 ; i++ ){
        if ( (ret = getFileLengthSub( &archive_info2[i], file_name )) ) return ret;
    }
    
    // sar read
    if ( sar_flag ) return SarReader::getFileLength( file_name );

    return 0;
}

size_t NsaReader::getFile( const char *file_name, unsigned char *buffer, int *location )
{
    size_t ret;

    // direct read
    if ( ( ret = DirectReader::getFile( file_name, buffer, location ) ) ) return ret;

    // nsa read
    if ( (ret = getFileSub( &archive_info_nsa, file_name, buffer )) ){
        if ( location ) *location = ARCHIVE_TYPE_NSA;
        return ret;
    }

    // nsa? read
    for ( int i=0 ; i<num_of_nsa_archives ; i++ ){
        if ( (ret = getFileSub( &archive_info2[i], file_name, buffer )) ){
            if ( location ) *location = ARCHIVE_TYPE_NSA;
            return ret;
        }
    }

    // sar read
    if ( sar_flag ) return SarReader::getFile( file_name, buffer, location );

    return 0;
}

struct NsaReader::FileInfo NsaReader::getFileByIndex( unsigned int index )
{
    int i;
    
    if ( index < archive_info_nsa.num_of_files ) return archive_info_nsa.fi_list[index];
    index -= archive_info_nsa.num_of_files;

    for ( i=0 ; i<num_of_nsa_archives ; i++ ){
        if ( index < archive_info2[i].num_of_files ) return archive_info2[i].fi_list[index];
        index -= archive_info2[i].num_of_files;
    }
    fprintf( stderr, "NsaReader::getFileByIndex  Index %d is out of range\n", index );

    return archive_info.fi_list[0];
}
