/* This is based on minip12.
 */

/*  minip12.c - A minilam pkcs-12 implementation.
 *	Copyright (C) 2002 Free Software Foundation, Inc.
 *
 *  This file some day was part of GnuPG.
 *  This file is part of GNUTLS.
 *
 *  The GNUTLS library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public   
 *  License as published by the Free Software Foundation; either 
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 */

#include <gnutls_int.h>

#ifdef ENABLE_PKI

#include <gcrypt.h>
#include <gnutls_errors.h>
#include <ctype.h>

/* Returns 0 if the password is ok, or a negative error
 * code instead.
 */
int _pkcs12_check_pass( const char* pass, size_t plen) 
{
const unsigned char* p = pass;
unsigned int i;

	for (i=0;i<plen;i++) {
		if ( p[i] < 128) continue;
		return GNUTLS_E_INVALID_PASSWORD;
	}
	
	return 0;
}

/* ID should be:
 * 3 for MAC
 * 2 for IV
 * 1 for encryption key
 */
int 
_pkcs12_string_to_key (unsigned int id, const opaque *salt, unsigned int salt_size, 
	unsigned int iter, const char *pw,
	unsigned int req_keylen, opaque *keybuf)
{
  int rc;
  unsigned int i, j;
  GcryMDHd md;
  GcryMPI num_b1 = NULL;
  unsigned int pwlen;
  opaque hash[20], buf_b[64], buf_i[128], *p;
  size_t cur_keylen;
  size_t n;

  cur_keylen = 0;
  pwlen = strlen (pw);
  if (pwlen > 63/2) {
      gnutls_assert();
      return GNUTLS_E_INVALID_REQUEST;
  }

  if ((rc=_pkcs12_check_pass( pw, pwlen)) < 0) {
  	gnutls_assert();
  	return rc;
  }

  /* Store salt and password in BUF_I */
  p = buf_i;
  for(i=0; i < 64; i++)
    *p++ = salt [i % salt_size];
  for(i=j=0; i < 64; i += 2)
    {
      *p++ = 0;
      *p++ = pw[j];
      if (++j > pwlen) /* Note, that we include the trailing zero */
        j = 0;
    }

  for (;;)
    {
      md = gcry_md_open (GCRY_MD_SHA1, 0);
      if (!md)
        {
          gnutls_assert();
          return GNUTLS_E_DECRYPTION_FAILED;
        }
      for(i=0; i < 64; i++)
        gcry_md_putc (md, id);
      gcry_md_write (md, buf_i, 128);
      memcpy (hash, gcry_md_read (md, 0), 20);
      gcry_md_close (md);
      for (i=1; i < iter; i++)
        gcry_md_hash_buffer (GCRY_MD_SHA1, hash, hash, 20);

      for (i=0; i < 20 && cur_keylen < req_keylen; i++)
        keybuf[cur_keylen++] = hash[i];
      if (cur_keylen == req_keylen)
        {
          gcry_mpi_release (num_b1);
          return 0; /* ready */
        }
      
      /* need more bytes. */
      for(i=0; i < 64; i++)
        buf_b[i] = hash[i % 20];
      n = 64;
      rc = gcry_mpi_scan (&num_b1, GCRYMPI_FMT_USG, buf_b, &n);
      if (rc)
        {
          gnutls_assert();
          return GNUTLS_E_DECRYPTION_FAILED;
        }
      gcry_mpi_add_ui (num_b1, num_b1, 1);
      for (i=0; i < 128; i += 64)
        {
          GcryMPI num_ij;

          n = 64;
          rc = gcry_mpi_scan (&num_ij, GCRYMPI_FMT_USG, buf_i + i, &n);
          if (rc)
            {
              gnutls_assert();
              return GNUTLS_E_DECRYPTION_FAILED;
            }
          gcry_mpi_add (num_ij, num_ij, num_b1);
          gcry_mpi_clear_highbit (num_ij, 64*8);
          n = 64;
          rc = gcry_mpi_print (GCRYMPI_FMT_USG, buf_i + i, &n, num_ij);
          if (rc)
            {
              gnutls_assert();
              return GNUTLS_E_DECRYPTION_FAILED;
            }
          gcry_mpi_release (num_ij);
        }
    }
}

#endif /* ENABLE_PKI */
