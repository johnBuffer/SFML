/* rbSFML - Copyright (c) 2010 Henrik Valter Vogelius Hansson - groogy@groogy.se
 * This software is provided 'as-is', without any express or
 * implied warranty. In no event will the authors be held
 * liable for any damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute
 * it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented;
 *    you must not claim that you wrote the original software.
 *    If you use this software in a product, an acknowledgment
 *    in the product documentation would be appreciated but
 *    is not required.
 *
 * 2. Altered source versions must be plainly marked as such,
 *    and must not be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any
 *    source distribution.
 */
 
#include "SoundStream.hpp"
#include "main.hpp"
#include <SFML/Audio/SoundStream.hpp>

VALUE globalSoundStreamClass;

/* External classes */
extern VALUE globalSoundSourceClass;

class rbSoundStream : public sf::SoundStream
{
public:
	rbSoundStream()
	{
	}
	
	~rbSoundStream()
	{
		if( myData != NULL )
		{
			delete[] myData;
		}
	}
	
	void Init( VALUE rubySelf )
	{
		mySelf = rubySelf;
		myOnGetDataID = rb_intern( "onGetData" );
		myOnSeekID = rb_intern( "onSeek" );
		
		myData = NULL;
	}
	
	void Initialize ( unsigned int channelsCount, unsigned int sampleRate )
	{
		sf::SoundStream::Initialize( channelsCount, sampleRate );
	} 	
	
protected:
	virtual bool OnGetData( Chunk& aData )
	{
		if( myData != NULL )
		{
			delete[] myData;
			myData = NULL;
		}
		
		VALUE chunk = rb_funcall( mySelf, myOnGetDataID, 0 );
		if( chunk == Qnil )
		{
			return false;
		}
		else
		{
			VALIDATE_CLASS( chunk, rb_cArray, "chunk" );
			const unsigned int rawSamplesCount = FIX2UINT( rb_funcall( chunk, rb_intern( "size" ), 0 ) );
			myData = new sf::Int16[rawSamplesCount];
			VALUE samples = rb_ary_entry( chunk, 0 );
			for(unsigned long index = 0; index < rawSamplesCount; index++)
			{
				const sf::Int16 val = NUM2INT( rb_ary_entry( samples, index ) );
				myData[index] = val;
			}
			aData.Samples = myData;
			aData.NbSamples = rawSamplesCount;
			return true;
		}
	}
	
	virtual void OnSeek( float anOffset )
	{
		rb_funcall( mySelf, myOnSeekID, 1, rb_float_new( anOffset ) );
	}
	
	VALUE mySelf;
	ID myOnGetDataID;
	ID myOnSeekID;
	
	sf::Int16 *myData;
};

static VALUE SoundStream_Free( rbSoundStream *anObject )
{
	delete anObject;
}

static VALUE SoundStream_Play( VALUE self )
{
	sf::SoundStream *object = NULL;
	Data_Get_Struct( self, sf::SoundStream, object );
	object->Play();
	return Qnil;
}

static VALUE SoundStream_Pause( VALUE self )
{
	sf::SoundStream *object = NULL;
	Data_Get_Struct( self, sf::SoundStream, object );
	object->Pause();
	return Qnil;
}

static VALUE SoundStream_Stop( VALUE self )
{
	sf::SoundStream *object = NULL;
	Data_Get_Struct( self, sf::SoundStream, object );
	object->Stop();
	return Qnil;
}

static VALUE SoundStream_GetChannelsCount( VALUE self )
{
	sf::SoundStream *object = NULL;
	Data_Get_Struct( self, sf::SoundStream, object );
	return INT2FIX( object->GetChannelsCount() );
}

static VALUE SoundStream_GetSampleRate( VALUE self )
{
	sf::SoundStream *object = NULL;
	Data_Get_Struct( self, sf::SoundStream, object );
	return INT2FIX( object->GetSampleRate() );
}

static VALUE SoundStream_GetStatus( VALUE self )
{
	sf::SoundStream *object = NULL;
	Data_Get_Struct( self, sf::SoundStream, object );
	return INT2FIX( static_cast< int >( object->GetStatus() ) );
}

static VALUE SoundStream_SetPlayingOffset( VALUE self, VALUE anOffset )
{
	sf::SoundStream *object = NULL;
	Data_Get_Struct( self, sf::SoundStream, object );
	object->SetPlayingOffset( NUM2DBL( anOffset ) );
	return Qnil;
}

static VALUE SoundStream_GetPlayingOffset( VALUE self, VALUE anOffset )
{
	sf::SoundStream *object = NULL;
	Data_Get_Struct( self, sf::SoundStream, object );
	return rb_float_new( object->GetPlayingOffset() );
}

static VALUE SoundStream_SetLoop( VALUE self, VALUE aLoop )
{
	sf::SoundStream *object = NULL;
	Data_Get_Struct( self, sf::SoundStream, object );
	if( aLoop == Qtrue )
	{
		object->SetLoop( true );
	}
	else if( aLoop == Qfalse )
	{
		object->SetLoop( false );
	}
	else
	{
		VALIDATE_CLASS( aLoop, rb_cTrueClass, "loop" );
	}
	return Qnil;
}

static VALUE SoundStream_GetLoop( VALUE self )
{
	sf::SoundStream *object = NULL;
	Data_Get_Struct( self, sf::SoundStream, object );
	if( object->GetLoop() == true )
	{
		return Qtrue;
	}
	else
	{
		return Qfalse;
	}
}

static VALUE SoundStream_Initialize( VALUE self, VALUE channelsCount, VALUE sampleRate )
{
	rbSoundStream *object = NULL;
	Data_Get_Struct( self, rbSoundStream, object );
	object->Initialize( FIX2UINT( channelsCount ), FIX2UINT( sampleRate ) );
	return self;
}

static VALUE SoundStream_New( int argc, VALUE *args, VALUE aKlass )
{
	rbSoundStream *object = new rbSoundStream();
	VALUE rbData = Data_Wrap_Struct( aKlass, 0, SoundStream_Free, object );
	object->Init( rbData );
	rb_obj_call_init( rbData, argc, args );
	return rbData;
}

void Init_SoundStream( void )
{
/* SFML namespace which contains the classes of this module. */
	VALUE sfml = rb_define_module( "SFML" );
/* Abstract base class for streamed audio sources.
 *
 * Unlike audio buffers (see SFML::SoundBuffer), audio streams are never completely loaded in memory.
 *
 * Instead, the audio data is acquired continuously while the stream is playing. This behaviour allows to play a sound
 * with no loading delay, and keeps the memory consumption very low.
 *
 * Sound sources that need to be streamed are usually big files (compressed audio musics that would eat hundreds of MB
 * in memory) or files that would take a lot of time to be received (sounds played over the network).
 * 
 * SFML::SoundStream is a base class that doesn't care about the stream source, which is left to the derived class. 
 * SFML provides a built-in specialization for big files (see SFML::Music). No network stream source is provided, but you
 * can write your own by combining this class with the network module.
 *
 * A derived class has to override two virtual functions:
 *
 *   - onGetData fills a new chunk of audio data to be played
 *   - onSeek changes the current playing position in the source
 *
 * It is important to note that each SoundStream is played in its own separate thread, so that the streaming loop 
 * doesn't block the rest of the program. In particular, the onGetData and onSeek virtual functions may sometimes be 
 * called from this separate thread. It is important to keep this in mind, because you may have to take care of 
 * synchronization issues if you share data between threads.
 *
 * Usage example:
 *
 *   class CustomStream < SFML::SoundStream
 *     def open( location )
 *       # Open the source and get audio settings
 *       ...
 *       channelsCount = ...
 *       sampleRate = ...
 *
 *       # Initialize the stream -- important!
 *       initialize( channelsCount, sampleRate )
 *     end
 *
 *     def onGetData( data )
 *       # Fill the chunk with audio data from the stream source
 *       data.Samples = ...;
 *       data.NbSamples = ...;
 *
 *       # Return true to continue playing
 *       return true;
 *     end
 *
 *     def onSeek( timeOffset )
 *       # Change the current position in the stream source
 *       ...
 *     end
 *   end
 *
 * # Usage
 * CustomStream stream;
 * stream.open( "path/to/stream" )
 * stream.play
 */
	globalSoundStreamClass = rb_define_class_under( sfml, "SoundStream", globalSoundSourceClass );

	// Class methods
	rb_define_singleton_method( globalSoundStreamClass, "new", SoundStream_New, -1 );
	
	// Instance methods
	rb_define_method( globalSoundStreamClass, "initialize", SoundStream_Initialize, 2 );
	rb_define_method( globalSoundStreamClass, "play", SoundStream_Play, 0 );
	rb_define_method( globalSoundStreamClass, "pause", SoundStream_Pause, 0 );
	rb_define_method( globalSoundStreamClass, "stop", SoundStream_Stop, 0 );
	rb_define_method( globalSoundStreamClass, "getChannelsCount", SoundStream_GetChannelsCount, 0 );
	rb_define_method( globalSoundStreamClass, "getSampleRate", SoundStream_GetSampleRate, 0 );
	rb_define_method( globalSoundStreamClass, "getStatus", SoundStream_GetStatus, 0 );
	rb_define_method( globalSoundStreamClass, "setPlayingOffset", SoundStream_SetPlayingOffset, 1 );
	rb_define_method( globalSoundStreamClass, "getPlayingOffset", SoundStream_GetPlayingOffset, 0 );
	rb_define_method( globalSoundStreamClass, "setLoop", SoundStream_SetLoop, 1 );
	rb_define_method( globalSoundStreamClass, "getLoop", SoundStream_GetLoop, 0 );
		
	// Instance Aliases
	rb_define_alias( globalSoundStreamClass, "get_channels_count", "getChannelsCount" );
	rb_define_alias( globalSoundStreamClass, "channelsCount", "getChannelsCount" );
	rb_define_alias( globalSoundStreamClass, "channels_count", "getChannelsCount" );
	
	rb_define_alias( globalSoundStreamClass, "get_sample_rate", "getSampleRate" );
	rb_define_alias( globalSoundStreamClass, "sampleRate", "getSampleRate" );
	rb_define_alias( globalSoundStreamClass, "sample_rate", "getSampleRate" );
	
	rb_define_alias( globalSoundStreamClass, "status", "getStatus" );
	
	rb_define_alias( globalSoundStreamClass, "get_playing_offset", "getPlayingOffset" );
	rb_define_alias( globalSoundStreamClass, "playingOffset", "getPlayingOffset" );
	rb_define_alias( globalSoundStreamClass, "playing_offset", "getPlayingOffset" );
	
	rb_define_alias( globalSoundStreamClass, "set_playing_offset", "setPlayingOffset" );
	rb_define_alias( globalSoundStreamClass, "playingOffset=", "setPlayingOffset" );
	rb_define_alias( globalSoundStreamClass, "playing_offset=", "setPlayingOffset" );
	
	rb_define_alias( globalSoundStreamClass, "loop", "getPlayingOffset" );
	rb_define_alias( globalSoundStreamClass, "loop=", "setPlayingOffset" );
}
