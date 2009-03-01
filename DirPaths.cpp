/* -*- C++ -*-
 * 
 *  DirPaths.cpp - contains multiple directory paths
 *
 *  Added by Sonozaki Futago-tachi, Nov. 2007
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

#include "DirPaths.h"

DirPaths::DirPaths()
{
    num_paths = 0;
    paths = NULL;
    all_paths = NULL;
}

DirPaths::DirPaths( const char *new_paths )
{
    num_paths = 0;
    paths = NULL;
    all_paths = NULL;
    add(new_paths);
}

DirPaths::DirPaths( DirPaths& old_dp )
{
    num_paths = 0;
    paths = NULL;
    add(old_dp.get_all_paths());
}

DirPaths::~DirPaths()
{
    if (paths != NULL) {
        char **ptr = paths;
        for (int i=0; i<num_paths; i++) {
            if (*ptr != NULL) {
                delete[] *ptr;
                *ptr = NULL;
            }
            ptr++;
        }
        delete[] paths;
        paths = NULL;
    }
    if (all_paths != NULL) {
        delete[] all_paths;
        all_paths = NULL;
    }
}

void DirPaths::add( const char *new_paths )
{
    fprintf(stderr, "Adding: %s\n", new_paths);
    
    char **old_paths;
    const char *ptr1, *ptr2;
    char *dptr;
    int n, i;

    if (all_paths != NULL) delete[] all_paths;
    all_paths = NULL;
    n = num_paths;
    num_paths++;
    ptr1 = new_paths;
    while (*ptr1 != '\0') {
        if (*ptr1++ == PATH_DELIMITER) {
            num_paths++;
        }
    }

    if (n > 0) {
        // make a new "paths", copy over existing ones
        //printf("DirPaths::add(\"%s\")\n", new_paths);
        old_paths = paths;
        paths = new char*[num_paths + 1];
        for (i=0; i<n; i++) {
            paths[i] = old_paths[i];
        }
        delete[] old_paths;
    } else {
        //printf("DirPaths(\"%s\")\n", new_paths);
        paths = new char*[num_paths + 1];
    }
    ptr1 = ptr2 = new_paths;
    do {
        while ((*ptr2 != '\0') && (*ptr2 != PATH_DELIMITER)) ptr2++;
        if (ptr2 == ptr1) {
            if (*ptr2 == '\0') break;
            ptr1++;
            ptr2++;
            continue;
        } else {
            paths[n] = new char[ptr2 - ptr1 + 2];
            dptr = paths[n];
            while (ptr1 != ptr2) *dptr++ = *ptr1++;
            if (*(ptr2 - 1) != DELIMITER) {
                // put a slash on the end if there isn't one already
                *dptr++ = DELIMITER;
            }
            *dptr = '\0';
        }
        if (*ptr2 != '\0') {
            ptr1++;
            ptr2++;
        }
        for (i=0; i<n; i++) {
            if (strcmp(paths[i],paths[n]) == 0)
                break;
        }
        if (i < n) {
            //printf("path already exists: \"%s\"\n", paths[i]);
            delete[] paths[n];
        } else {
            //printf("added path: \"%s\"\n", paths[n]);
            n++;
        }
    } while (*ptr2 != '\0');
    if (n == 0) {
        // need at least something for a path
        n = 1;
        if (paths != NULL) delete[] paths;
        paths = new char*[2];
        paths[0] = new char[1];
        *(paths[0]) = '\0';
    }
    num_paths = n;
    paths[num_paths] = NULL;
}

char* DirPaths::get_path( int n )
{
    if ((n > num_paths) || (n < 0))
        return NULL;
    else
        return paths[n];
}

// Returns a delimited string containing all paths
char* DirPaths::get_all_paths()
{
    if ((all_paths == NULL) && (paths != NULL)) {
        // construct all_paths
        char *buf;
        int n, len = 0;
        for (n=0; n<num_paths; n++) {
            if (paths[n] != NULL) len += strlen(paths[n]) + 1;
        }
        all_paths = new char[len];
        buf = new char[len];
        *buf = '\0';
        for (n=0; n<(num_paths-1); n++) {
            if (paths[n] != NULL) {
                sprintf(all_paths, "%s%s%c", buf, paths[n], PATH_DELIMITER);
                strcpy(buf, all_paths);
            }
        }
        sprintf(all_paths, "%s%s", buf, paths[n]);
        delete[] buf;
    }
    return all_paths;
}

int DirPaths::get_num_paths()
{
    return num_paths;
}

// Returns the length of the longest path
unsigned int DirPaths::max_path_len()
{
    unsigned int len = 0;
    for (int i=0; i<num_paths;i++) {
        if (strlen(paths[i]) > len)
            len = strlen(paths[i]);
    }
    return len;
}
