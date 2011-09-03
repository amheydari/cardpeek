/********************************************************************** 
*
* This file is part of Cardpeek, the smartcard reader utility.
*
* Copyright 2009 by 'L1L1'
*
* Cardpeek is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Cardpeek is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Cardpeek.  If not, see <http://www.gnu.org/licenses/>.
*
*/

/* #include "misc.h" */
#include "bytestring.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int bytestring_init(bytestring_t *bs, unsigned element_width)
{
  unsigned char mask;

  if (bs==NULL)
    return BYTESTRING_ERROR;

  switch (element_width) {
    case 1: mask=1; break;
    case 4: mask=0x0F; break;
    case 8: mask=0xFF; break;
    default: return BYTESTRING_ERROR;
  }

  bs->len=0;
  bs->alloc=0;
  bs->data=NULL;
  bs->width=element_width;
  bs->mask=mask;
  return BYTESTRING_OK;
}

bytestring_t* bytestring_new(unsigned element_width)
{
  bytestring_t *res=(bytestring_t*)malloc(sizeof(bytestring_t));

  if (bytestring_init(res,element_width)!=BYTESTRING_OK)
  {
    if (res) free(res);
    return NULL;
  }
  return res;
}

unsigned hex_nibble(const char nibble)
{
  if (nibble<='9' && nibble>='0')
    return nibble-'0';
  if (nibble<='F' && nibble>='A')
    return nibble-'A'+10;
  if (nibble<='f' && nibble>='a')
    return nibble-'a'+10;
  return BYTESTRING_NPOS;
}

bytestring_t* bytestring_new_from_string(unsigned element_width, const char *str)
{
  bytestring_t *dat=bytestring_new(element_width);
  unsigned str_len=strlen(str);
  unsigned c;
  unsigned value;
  unsigned state;
  size_t i=0;

  if (element_width==8)
  {
    /*bytestring_resize(dat,str_len/2);*/
    state=0;
    value=0;
    for (i=0;i<str_len;i++)
    {
      c=hex_nibble(str[i]);
      if (c==BYTESTRING_NPOS)
	continue;
      value=(value<<4)|c;
      state++;
      if (state==2)
      {	
  	bytestring_pushback(dat,(unsigned char)value);
	state=0;
	value=0;
      }
    }
    if (state==1)
      bytestring_pushback(dat,(unsigned char)(value<<4));
  }
  else
  {
    /*testring_resize(dat,str_len);*/
    for (i=0;i<str_len;i++)
    {
      value=hex_nibble(str[i]);
      if (value==BYTESTRING_NPOS)
	continue;
      bytestring_pushback(dat,(unsigned char)value);
    }
  }
  return dat; 
}

bytestring_t* bytestring_duplicate(const bytestring_t *bs)
{
  bytestring_t* res=bytestring_new(bs->width);
  bytestring_assign_data(res,bs->len,bs->data);
  return res;
}

int bytestring_assign_data(bytestring_t* bs,
                        unsigned len, const unsigned char *data)
{
  unsigned i;
  bytestring_resize(bs,len);
  for (i=0;i<len;i++)
    bs->data[i]=data[i]&bs->mask;
  return BYTESTRING_OK;
}

int bytestring_assign_element(bytestring_t* bs,
			      unsigned len, unsigned char c)
{
  bytestring_resize(bs,len);
  memset(bs->data,c&bs->mask,len);
  return BYTESTRING_OK;
}

int bytestring_copy(bytestring_t *bs,
                    const bytestring_t *src)
{
  if (bs!=src)
  {
    bs->width=src->width;
    return bytestring_assign_data(bs,src->len,src->data);
  }
  return BYTESTRING_OK;
}

int bs_convert_8_to_4(bytestring_t *bs,
		      const bytestring_t *src)
{
  unsigned u;
  bytestring_resize(bs,src->len*2);
  for (u=0;u<src->len;u++)
  {
    bs->data[u*2]=src->data[u]>>4;
    bs->data[u*2+1]=src->data[u]&&0xF;
  }
  return BYTESTRING_OK;
}

int bs_convert_8_to_1(bytestring_t *bs,
		      const bytestring_t *src)
{
  unsigned u;
  unsigned char c;

  bytestring_resize(bs,src->len*8);
  for (u=0;u<src->len;u++)
  {
    c=src->data[u];
    bs->data[u*8]=((c&128)>>7)&0x1;
    bs->data[u*8+1]=((c&64)>>6)&0x1;
    bs->data[u*8+2]=((c&32)>>5)&0x1;
    bs->data[u*8+3]=((c&16)>>4)&0x1;
    bs->data[u*8+4]=((c&8)>>3)&0x1;
    bs->data[u*8+5]=((c&4)>>2)&0x1;
    bs->data[u*8+6]=((c&2)>>1)&0x1;
    bs->data[u*8+7]=c&0x1;
  }
  return BYTESTRING_OK;
}

int bs_convert_4_to_1(bytestring_t *bs,
		      const bytestring_t *src)
{
  unsigned u;
  unsigned char c;

  bytestring_resize(bs,src->len*4);
  for (u=0;u<src->len;u++)
  {
    c=src->data[u];
    bs->data[u*4]=((c&8)>>3)&0x1;
    bs->data[u*4+1]=((c&4)>>2)&0x1;
    bs->data[u*4+2]=((c&2)>>1)&0x1;
    bs->data[u*4+3]=c&0x1;
  }
  return BYTESTRING_OK;
}

int bs_convert_4_to_8(bytestring_t *bs,
		      const bytestring_t *src)
{
  unsigned des_i;
  unsigned src_i;
  bytestring_resize(bs,(src->len+1)/2);

  if ((src->len&1)==1)
  {
    bs->data[0]=src->data[0];
    des_i=1;
    src_i=1;
  }
  else
  {
    des_i=0;
    src_i=0;
  }

  while (src_i<src->len)
  {
    bs->data[des_i]=src->data[src_i]<<4;
    bs->data[des_i]+=src->data[src_i+1];
    des_i+=1;
    src_i+=2;
  }
  return BYTESTRING_OK;
}

int bs_convert_1_to_8(bytestring_t *bs,
		      const bytestring_t *src)
{
  unsigned src_i;
  unsigned dst_i;
  bytestring_resize(bs,(src->len+7)/8);

  if ((src->len&7)!=0)
  {
    bs->data[0]=0;
    for (src_i=0;src_i<(src->len&7);src_i++)
    {
      bs->data[0]<<=1;
      bs->data[0]|=src->data[src_i];
    }
    dst_i=1;
  }
  else
  {
    src_i=0;
    dst_i=0;
  }

  while (src_i<src->len)
  {
    bs->data[dst_i] =src->data[src_i++]<<7;
    bs->data[dst_i]|=src->data[src_i++]<<6;
    bs->data[dst_i]|=src->data[src_i++]<<5;
    bs->data[dst_i]|=src->data[src_i++]<<4;
    bs->data[dst_i]|=src->data[src_i++]<<3;
    bs->data[dst_i]|=src->data[src_i++]<<2;
    bs->data[dst_i]|=src->data[src_i++]<<1;
    bs->data[dst_i]|=src->data[src_i++];
    dst_i++;
  }
  return BYTESTRING_OK;
}

int bs_convert_1_to_4(bytestring_t *bs,
		      const bytestring_t *src)
{
  unsigned src_i;
  unsigned dst_i;
  bytestring_resize(bs,(src->len+3)/4);

  if ((src->len&3)!=0)
  {
    bs->data[0]=0;
    for (src_i=0;src_i<(src->len&3);src_i++)
    {
      bs->data[0]<<=1;
      bs->data[0]|=src->data[src_i];
    }
    dst_i=1;
  }
  else
  {
    src_i=0;
    dst_i=0;
  }

  while (src_i<src->len)
  {
    bs->data[dst_i]|=src->data[src_i++]<<3;
    bs->data[dst_i]|=src->data[src_i++]<<2;
    bs->data[dst_i]|=src->data[src_i++]<<1;
    bs->data[dst_i]|=src->data[src_i++];
    dst_i++;
  }
  return BYTESTRING_OK;
}

int bytestring_convert(bytestring_t *bs,
		       const bytestring_t *src)
{
  unsigned algo;
  bytestring_t* tmp;
  int retval;

  if (bs==src)
  {
    tmp=bytestring_duplicate(src);
    retval=bytestring_convert(bs,tmp);
    bytestring_free(tmp);
    return retval;
  }

  algo=src->width*10+bs->width;

  switch (algo) {
    case 11:
    case 44:
    case 88:
      return bytestring_copy(bs,src);
    case 14:
      return bs_convert_1_to_4(bs,src);
    case 18:
      return bs_convert_1_to_8(bs,src);
    case 41:
      return bs_convert_4_to_1(bs,src);
    case 48:
      return bs_convert_4_to_8(bs,src);
    case 81:
      return bs_convert_8_to_1(bs,src);
    case 84:
      return bs_convert_8_to_4(bs,src);
  }
  return BYTESTRING_ERROR;
}


int bytestring_append(bytestring_t *bs,
                      const bytestring_t *extra)
{
  bytestring_t* tmp;
  int retval;

  if (bs==extra)
  {
    tmp=bytestring_duplicate(extra);
    retval=bytestring_append(bs,tmp);
    bytestring_free(tmp);
    return retval;
  }
  return bytestring_append_data(bs,extra->len,extra->data);
}

int bytestring_append_data(bytestring_t *bs,
                           unsigned len, const unsigned char *data)
{
  unsigned i;
  unsigned old_len=bs->len;

  bytestring_resize(bs,bs->len+len);
  for (i=0;i<len;i++)
    bs->data[old_len+i]=data[i]&bs->mask;
  return BYTESTRING_OK;
}

int bytestring_append_element(bytestring_t* bs,
			      unsigned len, unsigned char c)
{
  unsigned i;
  unsigned old_len=bs->len;

  bytestring_resize(bs,bs->len+len);
  for (i=0;i<len;i++)
    bs->data[old_len+i]=c&bs->mask;
  return BYTESTRING_OK;
}

int bytestring_pushback(bytestring_t *bs,
                        unsigned char c)
{
  unsigned old_len=bs->len;

  bytestring_resize(bs,bs->len+1);
  bs->data[old_len]=c&bs->mask;
  return BYTESTRING_OK;
}

int bytestring_get_element(unsigned char* element,
			   const bytestring_t *bs,
			   int pos)
{
  if (pos<0)
    pos=bs->len+pos;
  if (pos<bs->len && pos>=0)
  {
    *element=bs->data[pos];
    return BYTESTRING_OK;
  }
  *element=0;
  return BYTESTRING_ERROR;
}

int bytestring_set_element(const bytestring_t *bs,
			   int pos, unsigned char element)
{
  if (pos<0)
    pos=bs->len+pos;
  if (pos<bs->len && pos>=0)
  {
    bs->data[pos]=element&bs->mask;
    return BYTESTRING_OK;
  }
  return BYTESTRING_ERROR;
}

int bytestring_invert(bytestring_t *bs)
{
  unsigned last=bs->len-1;
  unsigned c;
  unsigned char extra;

  for (c=0;c<bs->len/2;c++)
  {
    extra=bs->data[last-c];
    bs->data[last-c]=bs->data[c];
    bs->data[c]=extra;
  }
  return BYTESTRING_OK;
}

void bytestring_clear(bytestring_t *bs)
{
  bs->len=0;
}

const unsigned char *bytestring_get_data(const bytestring_t *bs)
{
  return bs->data;
}

int bytestring_erase(bytestring_t *bs,
                     unsigned pos,
                     unsigned len)
{
  if (pos>=bs->len)
    return BYTESTRING_OK;
  if (pos+len>=bs->len) 
    return bytestring_resize(bs,pos);
  memmove(bs->data+pos,bs->data+pos+len,bs->len-pos-len);
  bytestring_resize(bs,bs->len-len);
  return BYTESTRING_OK;
}

int bytestring_is_equal(const bytestring_t *a, const bytestring_t *b)
{
  unsigned u;
  if (a->width!=b->width)
    return 0;
  if (a->len!=b->len)
    return 0;
  for (u=0;u<a->len;u++)
    if (a->data[u]!=b->data[u]) return 0;
  return 1;
}

int bytestring_is_empty(const bytestring_t *bs)
{
  return bs->len==0;
}

int bytestring_is_printable(const bytestring_t *bs)
{
  unsigned u;

  if (bs->len==0)
    return 0;
  for (u=0;u<bs->len;u++)
  {
    if (!isprint((char)(bs->data[u]))) return 0;
  }
  return 1;
}

int bytestring_insert_data(bytestring_t *bs,
                           unsigned pos,
                           unsigned len, const unsigned char* data)
{
  unsigned u;

  if (pos>=bs->len)
    return bytestring_append_data(bs,len,data);
  bytestring_resize(bs,bs->len+len);
  memmove(bs->data+pos+len,bs->data+pos,bs->len-pos);
  for (u=0;u<len;u++)
    bs->data[pos+u]=data[u]&bs->mask;
  return BYTESTRING_OK;
}

int bytestring_insert_element(bytestring_t *bs,
			      unsigned pos,
			      unsigned len, unsigned char c)
{
  unsigned u;

  if (pos>=bs->len)
    return bytestring_append_element(bs,len,c);
  bytestring_resize(bs,bs->len+len);
  memmove(bs->data+pos+len,bs->data+pos,len);
  for (u=0;u<len;u++)
    bs->data[pos+u]=c&bs->mask;
  return BYTESTRING_OK;
}

int bytestring_insert(bytestring_t *bs,
                      unsigned pos,
                      const bytestring_t *src)
{
  bytestring_t* tmp;
  int retval;

  if (bs==src)
  {
    tmp=bytestring_duplicate(src);
    retval=bytestring_insert(bs,pos,tmp);
    bytestring_free(tmp);
    return retval;
  }

  return bytestring_insert_data(bs,pos,src->len,src->data);
}

int bytestring_pad_left(bytestring_t *bs,
                        unsigned block_size, unsigned char c)
{
  unsigned pad = block_size-(bs->len%block_size);

  if (pad==block_size)
    return BYTESTRING_OK;
  return bytestring_insert_element(bs,0,pad,c);
}

int bytestring_pad_right(bytestring_t *bs,
                         unsigned block_size, unsigned char c)
{
  unsigned pad = block_size-(bs->len%block_size);

  if (pad==block_size)
    return BYTESTRING_OK;
  return bytestring_append_element(bs,pad,c);
}


int bytestring_resize(bytestring_t *bs, unsigned len)
{
  if (len>bs->len)
  {
    if (len>bs->alloc)
    {
      if (bs->alloc==0)
      {
	bs->alloc=(len<8?8:len);
	bs->data=(unsigned char *)malloc(bs->alloc);
      }
      else
      {
	while (bs->alloc<len) bs->alloc<<=1;
	bs->data=(unsigned char *)realloc(bs->data,bs->alloc);
	if (bs->data==NULL)
	{
	  bs->alloc=0;
	  return BYTESTRING_ERROR;
	}
      }
    }
  }
  bs->len=len;
  return BYTESTRING_OK;
}

unsigned bytestring_get_size(const bytestring_t *bs)
{
  return bs->len;
}

int bytestring_substr(bytestring_t* dst,
		      unsigned pos, unsigned len,
		      const bytestring_t *src)
{
  bytestring_t* tmp;
  int retval;

  if (dst==src)
  {
    tmp=bytestring_duplicate(src);
    retval=bytestring_substr(dst,pos,len,tmp);
    bytestring_free(tmp);
    return retval;
  }

  if (pos>src->len)
  {
    bytestring_clear(dst);
    return BYTESTRING_ERROR;
  }
  if (len==BYTESTRING_NPOS || pos+len>src->len)
    len = src->len-pos;
  return bytestring_assign_data(dst,len,src->data+pos);
}

char *bytestring_to_alloc_printable(const bytestring_t *bs)
{
  char *str=(char *)malloc(bs->len+1);
  unsigned u;
  for (u=0;u<bs->len;u++)
  {
    if (!isprint((char)(bs->data[u])))
      str[u]='?';
    else
      str[u]=(char)(bs->data[u]);
  }
  str[u]=0;
  return str;
}

const char HEXA[]="0123456789ABCDEF";

char *bytestring_to_alloc_string(const bytestring_t *bs)
{
  char *str;
  unsigned i;

  if (bs->width==8)
  {
    str=(char *)malloc(bs->len*2+1);

    for (i=0;i<bs->len;i++)
    {
      str[i<<1]=HEXA[(bs->data[i]>>4)&0xF];
      str[(i<<1)+1]=HEXA[bs->data[i]&0xF];
    }
    str[i<<1]=0;
  }
  else
  {
    str=(char *)malloc(bs->len+1);

    for (i=0;i<bs->len;i++)
      str[i]=HEXA[bs->data[i]&0xF];
    str[i]=0;
  }
  return str;
}

int x_bytestring_set(bytestring_t *bs, int index, unsigned char v)
{
  if (index>=bs->len) 
    bytestring_resize(bs,index+1);
  bs->data[index]=v;
  
  return BYTESTRING_OK;
}
unsigned x_bytestring_get(bytestring_t *bs, int index)
{
  if (index>=bs->len)
    return 0;
  return (unsigned)bs->data[index];
}

int x_bytestring_decimal_mul256_add(bytestring_t *bs, unsigned char v)
{
  unsigned r;
  int index;
  int i;

  for (index=bs->len-1;index>=0;index--)
  {
    r=((unsigned)bs->data[index])<<8;
    x_bytestring_set(bs,index,r%10);
    r/=10;
    i=1;
    while (r)
    {
      r+=x_bytestring_get(bs,index+i);
      if (x_bytestring_set(bs,index+i,r%10)!=BYTESTRING_OK)
	return BYTESTRING_ERROR;
      r/=10;
      i++;
    }
  }
  r=x_bytestring_get(bs,0)+v;
  x_bytestring_set(bs,0,r%10);
  r/=10;
  i=1;
  while (r)
  {
    r+=x_bytestring_get(bs,i);
    if (x_bytestring_set(bs,i,r%10)!=BYTESTRING_OK)
      return BYTESTRING_ERROR;
    r/=10;
    i++;
  }
  return BYTESTRING_OK;
}

char *bytestring_to_alloc_number(const bytestring_t *bs)
{
  bytestring_t *src;
  bytestring_t *b10;
  int i;
  char *ret;

  if (bs->len==0)
    return strdup("0");
  
  src = bytestring_new(8);
  b10 = bytestring_new(8);
  
  bytestring_convert(src,bs);
  bytestring_pushback(b10,0);
  
  for (i=0;i<src->len;i++)
    x_bytestring_decimal_mul256_add(b10,src->data[i]);
  
  ret=(char *)malloc(b10->len+1);
  for (i=0;i<b10->len;i++)
    ret[i]=b10->data[b10->len-1-i]+'0';
  ret[i]=0;
  
  bytestring_free(src);
  bytestring_free(b10);
  return ret;
}

void bytestring_release(bytestring_t *bs)
{
  if (bs->data)
    free(bs->data); 
  bs->data=NULL;
}

void bytestring_free(bytestring_t *bs)
{
  bytestring_release(bs);
  free(bs);
}


#ifdef BYTESTRING_TEST
#include <stdio.h>
int main()
{
  bytestring_t *bs;
  bytestring_t *bb;

  bs=bytestring_new_from_string(1,"0101");
  bb=bytestring_new(8);

  bytestring_pad_right(bs,32,1);
  printf("%d:%s\n",bytestring_get_size(bs),bytestring_to_alloc_string(bs));
  bytestring_convert(bb,bs);
  bytestring_invert(bb);
  printf("%d:%s\n",bytestring_get_size(bb),bytestring_to_alloc_string(bb));
  bytestring_free(bs);

  return 0;
}
#endif