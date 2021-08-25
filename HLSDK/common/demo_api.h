//
// Created by unko on 8/24/21.
//

#ifndef BUNNYMODXT_DEMO_API_H
#define BUNNYMODXT_DEMO_API_H
#ifdef _WIN32
#pragma once
#endif

typedef struct demo_api_s
{
	int		( *IsRecording )	( void );
	int		( *IsPlayingback )	( void );
	int		( *IsTimeDemo )		( void );
	void	( *WriteBuffer )	( int size, unsigned char *buffer );
} demo_api_t;

extern demo_api_t demoapi;

#endif //BUNNYMODXT_DEMO_API_H
