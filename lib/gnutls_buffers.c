#include <defines.h>
#include "gnutls_int.h"
#include "gnutls_errors.h"

int gnutls_insertDataBuffer(ContentType type, GNUTLS_STATE state, char *data, int length)
{
	int old_buffer;

	if (type == GNUTLS_APPLICATION_DATA) {
		old_buffer = state->gnutls_internals.bufferSize;

		state->gnutls_internals.bufferSize += length;
#ifdef HARD_DEBUG
	fprintf(stderr, "Inserted %d bytes of Data(%d) into buffer\n", length, type);
#endif
		state->gnutls_internals.buffer =
		    gnutls_realloc(state->gnutls_internals.buffer,
			   state->gnutls_internals.bufferSize);
		memmove(&state->gnutls_internals.buffer[old_buffer], data, length);
	}
	if (type == GNUTLS_HANDSHAKE) {
		old_buffer = state->gnutls_internals.bufferSize_handshake;

		state->gnutls_internals.bufferSize_handshake += length;
#ifdef HARD_DEBUG
	fprintf(stderr, "Inserted %d bytes of Data(%d) into buffer\n", length, type);
#endif
		state->gnutls_internals.buffer_handshake =
		    gnutls_realloc(state->gnutls_internals.buffer_handshake,
			   state->gnutls_internals.bufferSize_handshake);
		memmove(&state->gnutls_internals.buffer_handshake[old_buffer], data, length);
	}

	return 0;

}

int gnutls_getDataBufferSize(ContentType type, GNUTLS_STATE state)
{
	if (type == GNUTLS_APPLICATION_DATA)
		return state->gnutls_internals.bufferSize;
	if (type == GNUTLS_HANDSHAKE)
		return state->gnutls_internals.bufferSize_handshake;
	return 0;
}

int gnutls_getDataFromBuffer(ContentType type, GNUTLS_STATE state, char *data, int length)
{
	if (type == GNUTLS_APPLICATION_DATA) {
	
		if (length > state->gnutls_internals.bufferSize) {
			length = state->gnutls_internals.bufferSize;
		}
#ifdef HARD_DEBUG
	fprintf(stderr, "Read %d bytes of Data(%d) from buffer\n", length, type);
#endif
		state->gnutls_internals.bufferSize -= length;
		memmove(data, state->gnutls_internals.buffer, length);

		/* overwrite buffer */
		memmove(state->gnutls_internals.buffer,
			&state->gnutls_internals.buffer[length],
			state->gnutls_internals.bufferSize);
		state->gnutls_internals.buffer =
		    gnutls_realloc(state->gnutls_internals.buffer,
				   state->gnutls_internals.bufferSize);
	}
	if (type == GNUTLS_HANDSHAKE) {
		if (length > state->gnutls_internals.bufferSize_handshake) {
			length = state->gnutls_internals.bufferSize_handshake;
		}
#ifdef HARD_DEBUG
	fprintf(stderr, "Read %d bytes of Data(%d) from buffer\n", length, type);
#endif
		state->gnutls_internals.bufferSize_handshake -= length;
		memmove(data, state->gnutls_internals.buffer_handshake, length);

		/* overwrite buffer */
		memmove(state->gnutls_internals.buffer_handshake,
			&state->gnutls_internals.buffer_handshake[length],
			state->gnutls_internals.bufferSize_handshake);
		state->gnutls_internals.buffer_handshake =
		    gnutls_realloc(state->gnutls_internals.buffer_handshake,
				   state->gnutls_internals.bufferSize_handshake);
	}


	return length;
}

ssize_t Read(int fd, void *iptr, size_t sizeOfPtr)
{
	size_t left;
	ssize_t i=0;
	char *ptr = iptr;

	left = sizeOfPtr;
	while (left > 0) {
		i = read(fd, &ptr[i], left);
		if (i < 0) {
			return -1;
		} else {
			if (i == 0)
				break;	/* EOF */
		}

		left -= i;

	}

	return (sizeOfPtr - left);
}


ssize_t Write(int fd, const void *iptr, size_t n)
{
	size_t left;
	ssize_t i = 0;
	const char *ptr = iptr;

	left = n;
	while (left > 0) {
		i = write(fd, &ptr[i], left);
		if (i <= 0) {
			return -1;
		}
		left -= i;
	}

	return n;

}
ssize_t _gnutls_Send_int(int fd, GNUTLS_STATE state, ContentType type, void *iptr, size_t n)
{
	size_t left;
	ssize_t i = 0;
	char *ptr = iptr;

	left = n;
	while (left > 0) {
		i = gnutls_send_int(fd, state, type, &ptr[i], left);
		if (i <= 0) {
			return i;
		}
		left -= i;
	}

	return n;

}

ssize_t _gnutls_Recv_int(int fd, GNUTLS_STATE state, ContentType type, void *iptr, size_t sizeOfPtr)
{
	size_t left;
	ssize_t i=0;
	char *ptr = iptr;

	left = sizeOfPtr;
	while (left > 0) {
		i = gnutls_recv_int(fd, state, type, &ptr[i], left);
		if (i < 0) {
			return i;
		} else {
			if (i == 0)
				break;	/* EOF */
		}

		left -= i;

	}

	return (sizeOfPtr - left);
}
