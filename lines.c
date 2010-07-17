/*
  Copyright (C) 2008, 2009, 2010  Dan Davison

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, a copy is available at
  http://www.gnu.org/licenses/gpl.txt
*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#define MAX(x,y) ((x) >= (y) ? (x) : (y))

void ERROR (char *msg) {
    fputs(msg, stderr) ;
    fputc('\n', stderr) ;
    exit(2) ;
}

void usage() {
    ERROR("lines -f linenumfile < inputfile ") ;
}

void FSEEK_ERROR() {
    ERROR("fseek error (Did you use a pipe and ask for non-monotonically increasing lines? You can't use a pipe in that case.) :") ;
}

int main(int argc, char **argv) {
    FILE *lfile=NULL, *infile ;
    int wantline, currline, furthest, c ;
    size_t maxlinelength=0, maxnlines ;
    long *linepos ;
    char *line=NULL ; /* getline automatically mallocs and reallocs it */
    
    while((c = getopt(argc, argv, "f:")) != -1) {
	switch(c) {
	case 'f':
	    lfile = fopen(optarg, "r") ;
	    break ;
	case '?':
	    usage() ;
	}
    }
    if(lfile == NULL) usage() ;

    maxnlines = 1e3 ;
    linepos = (long *) malloc(maxnlines * sizeof(long)) ;
    for(c = 0 ; c < maxnlines ; c++) linepos[c] = -9 ;
    
    infile = stdin ; // NB can't use fseek with a pipe! See FSEEK_ERROR above.
    currline = furthest = 0 ;

    while( fscanf(lfile, "%d", &wantline) == 1) {
	/* Each time this loop is entered we have a new wantline, and
	   the task is to find it and print it */
	
	wantline-- ;  // indexing from 1 in shell
	linepos[currline] = ftell(infile) ;

	/* First check whether we need to rewind to get to wantline */
	if(wantline < currline) {
	    if( fseek(infile, linepos[wantline] - linepos[currline], SEEK_CUR) != 0 )
		FSEEK_ERROR() ;
	    if( getline(&line, &maxlinelength, infile) < 0 )
	    	ERROR("Failed to read new line (reached end of file?)") ;
	    printf("%s", line) ;
	    currline = wantline ;
	    continue ;
	}
	
	/* So we're either at wantline or it's ahead of us */
	if(furthest > currline) { /* If we've already been further than this point, 
				     than we can fast-forward to the relevant spot. */
	    if(wantline <= furthest) {  /* we've been past it previously, but subsequently did a rewind: 
					   go straight to it, print it and skip to next wantline */
		if( fseek(infile, linepos[wantline] - linepos[currline], SEEK_CUR) != 0 )
		    FSEEK_ERROR() ;
		if( getline(&line, &maxlinelength, infile) < 0 )
		    ERROR("Failed to read new line (reached end of file?)") ;
		printf("%s", line) ;
		currline = wantline + 1 ;
		continue ;
	    }
	    /* Wantline is further than we've been before: fast-forward to our furthest point, 
	       then start advancing line-by-line  */
	    if( fseek(infile, linepos[furthest] - linepos[currline], SEEK_CUR) != 0 )
		FSEEK_ERROR() ;
	    currline = furthest ;
	}

	/* Now we're going beyond where we've been before. 
	   So advance line-by-line, storing the positions of the new lines, until we find wantline */
	while(wantline > currline) {
	    if( getline(&line, &maxlinelength, infile) < 0 )
		ERROR("Failed to read new line (reached end of file?)") ;
	    currline++ ;
	    if(currline > maxnlines) {
		maxnlines *= 2 ;
		linepos = realloc(linepos, maxnlines * sizeof(long)) ;
	    }
	    linepos[currline] = ftell(infile) ;
	}
	
	/* We're at wantline */
	assert(currline == wantline) ;
	if( getline(&line, &maxlinelength, infile) < 0)
	    ERROR("Failed to read next line") ;
	printf("%s", line) ;
	currline++ ;
	if(currline > maxnlines) {
	    maxnlines *= 2 ;
	    linepos = realloc(linepos, maxnlines * sizeof(long)) ;
	}
	furthest = MAX(furthest, currline) ;
    }
    
    fclose(infile) ;
    fclose(lfile) ;
    free(line) ;
    free(linepos) ;
    return 0 ;
}
