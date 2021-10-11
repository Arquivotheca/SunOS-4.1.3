/*      @(#)check_part.c 1.1 92/07/30 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

int
check_partition(name)
	char name[];
{
	register ok, i;
	int unit;
	char str[5];

	ok = 1;
	if ( strlen(name) > 0 && strlen(name) < 6 ) {
		if ( !strncmp(name,"xy",2) ) {
			bzero(str, sizeof(str));
			for(i=2;i < strlen(name)-1;i++) {
				if ( isdigit(name[i]) ) {
					str[i-2] = name[i];
				} else {
					ok = 0;
					break;
				}
			}
			if ( ok ) {
				str[i] = '\0';
        			unit = atoi(str);
        			if ( unit >= 0 && unit < 4 ) 
					ok = 1;
				else
					ok = 0;
			}
        	} else if ( !strncmp(name,"ip",2) ) {
			bzero(str,sizeof(str));
			for(i=2;i < strlen(name)-1;i++) {
                                if ( isdigit(name[i]) ) { 
                                        str[i-2] = name[i];   
                                } else { 
                                        ok = 0;  
                                        break; 
                                } 
                        } 
                        if ( ok ) { 
                                str[i] = '\0';   
                                unit = atoi(str);
                                if ( unit >= 0 && unit < 8 ) 
                                        ok = 1;
                                else 
                                        ok = 0;  
                        }
        	} else if ( !strncmp(name,"xd",2) ) {
			bzero(str,sizeof(str));
			for(i=2;i < strlen(name)-1;i++) {
                                if ( isdigit(name[i]) ) { 
                                        str[i-2] = name[i];   
                                } else { 
                                        ok = 0;  
                                        break; 
                                } 
                        } 
                        if ( ok ) { 
                                str[i] = '\0';   
                                unit = atoi(str);
                                if ( unit >= 0 && unit < 16 ) 
                                        ok = 1;
                                else 
                                        ok = 0;  
                        }
        	} else if ( !strncmp(name,"sd",2) ) {
			bzero(str, sizeof(str));
			for(i=2;i < strlen(name)-1;i++) { 
                                if ( isdigit(name[i]) ) {  
                                        str[i-2] = name[i];    
                                } else {  
                                        ok = 0;   
                                        break;  
                                }  
                        }  
                        if ( ok ) { 
                                str[i] = '\0';    
                                unit = atoi(str); 
                                if ( unit >= 0 && unit < 3 )  
                                        ok = 1; 
                                else  
                                        ok = 0;   
                        }
        	} else {
			ok = 0;
        	}
        	if ( ok ) {
			if ( name[strlen(name)-1] == 'a' ||  
			     name[strlen(name)-1] == 'b' || 
			     name[strlen(name)-1] == 'c' ||  
			     name[strlen(name)-1] == 'd' || 
             	     	     name[strlen(name)-1] == 'e' ||  
			     name[strlen(name)-1] == 'f' || 
             	     	     name[strlen(name)-1] == 'g' ||  
			     name[strlen(name)-1] == 'h' ) {
			     	ok = 1;
			} else {
				ok = 0;
			}
        	}
	} else {
		ok = 0;
	}
	return (ok);
}
