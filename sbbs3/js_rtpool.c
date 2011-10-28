/* $Id$ */

#include "js_rtpool.h"
#include <threadwrap.h>		/* Must be included after jsapi.h */
#include <genwrap.h>		/* SLEEP() */

#ifdef DLLCALL
#undef DLLCALL
#endif
#ifdef _WIN32
	#ifdef __BORLANDC__
		#define DLLCALL __stdcall
	#else
		#define DLLCALL
	#endif
#else	/* !_WIN32 */
	#define DLLCALL
#endif

#define SHARED_RUNTIMES

struct jsrt_queue {
	JSRuntime       *rt;
	int			created;
#ifdef SHARED_RUNTIMES
	const char*	file;
	long		line;
#else
	int			maxbytes;
	int			used;
#endif
};

#define JSRT_QUEUE_SIZE		128
struct jsrt_queue jsrt_queue[JSRT_QUEUE_SIZE];
static pthread_mutex_t		jsrt_mutex;
static int			initialized=0;
#ifndef SHARED_RUNTIMES
static sem_t			jsrt_sem;
#endif

static void trigger_thread(void *args)
{
	int	i;

	for(;;) {
		pthread_mutex_lock(&jsrt_mutex);
		for(i=0; i<JSRT_QUEUE_SIZE; i++) {
			if(jsrt_queue[i].created)
				JS_TriggerAllOperationCallbacks(jsrt_queue[i].rt);
		}
		pthread_mutex_unlock(&jsrt_mutex);
		SLEEP(100);
	}
}

JSRuntime * DLLCALL jsrt_GetNew(int maxbytes, unsigned long timeout, const char *filename, long line)
{
#ifdef SHARED_RUNTIMES
	int	i;

	if(!initialized) {
		pthread_mutex_init(&jsrt_mutex, NULL);
		_beginthread(trigger_thread, 65536, NULL);
		initialized=TRUE;
	}

	pthread_mutex_lock(&jsrt_mutex);
	for(i=0; i<JSRT_QUEUE_SIZE; i++) {
		if(!jsrt_queue[i].created) {
			jsrt_queue[i].rt=JS_NewRuntime(maxbytes);
			if(jsrt_queue[i].rt != NULL) {
				jsrt_queue[i].file=filename;
				jsrt_queue[i].line=line;
				jsrt_queue[i].created=1;
			}
		}
		if(jsrt_queue[i].created && jsrt_queue[i].file == filename && jsrt_queue[i].line == line) {
			pthread_mutex_unlock(&jsrt_mutex);
			return(jsrt_queue[i].rt);
		}
	}
	pthread_mutex_unlock(&jsrt_mutex);

	return(NULL);
#else
	int	i;
	int	last_unused=-1;

	if(!initialized) {
		pthread_mutex_init(&jsrt_mutex, NULL);
		sem_init(&jsrt_sem, 0, JSRT_QUEUE_SIZE);
		initialized=TRUE;
	}

	if(sem_trywait_block(&jsrt_sem, timeout)==0) {
		pthread_mutex_lock(&jsrt_mutex);
		for(i=0; i<JSRT_QUEUE_SIZE; i++) {
			if(!jsrt_queue[i].created) {
				jsrt_queue[i].rt=JS_NewRuntime(maxbytes);
				if(jsrt_queue[i].rt != NULL) {
					jsrt_queue[i].maxbytes=maxbytes;
					jsrt_queue[i].created=1;
					jsrt_queue[i].used=0;
				}
			}
			if(!jsrt_queue[i].used) {
				last_unused=i;
				if(jsrt_queue[i].created && jsrt_queue[i].maxbytes == maxbytes) {
					jsrt_queue[i].used=1;
					pthread_mutex_unlock(&jsrt_mutex);
					return(jsrt_queue[i].rt);
				}
			}
		}

		if(last_unused != -1) {
			jsrt_queue[last_unused].used=1;
			pthread_mutex_unlock(&jsrt_mutex);
			return(jsrt_queue[last_unused].rt);
		}
		pthread_mutex_unlock(&jsrt_mutex);
	}

	return(NULL);
#endif
}

void DLLCALL jsrt_Release(JSRuntime *rt)
{
#ifdef SHARED_RUNTIMES
#else
	int	i;

	for(i=0; i<JSRT_QUEUE_SIZE; i++) {
		if(rt==jsrt_queue[i].rt) {
			pthread_mutex_lock(&jsrt_mutex);
			jsrt_queue[i].used=0;
			/* TODO: Clear "stuff"? */
			pthread_mutex_unlock(&jsrt_mutex);
			sem_post(&jsrt_sem);
		}
	}
#endif
}

void DLLCALL jsrt_TriggerAll(void)
{
#if JS_VERSION>180
	int	i;
	JSContext	*iterp,*cx;

	if(!initialized)
		return;
	for(i=0; i<JSRT_QUEUE_SIZE; i++) {
		pthread_mutex_lock(&jsrt_mutex);
#ifdef SHARED_RUNTIMES
		if(jsrt_queue[i].created) {
#else
		if(jsrt_queue[i].used) {
#endif
			iterp=NULL;
			while((cx = JS_ContextIterator(jsrt_queue[i].rt, &iterp)) != NULL)
				JS_TriggerOperationCallback(cx);
		}
		pthread_mutex_unlock(&jsrt_mutex);
	}
#endif
}
