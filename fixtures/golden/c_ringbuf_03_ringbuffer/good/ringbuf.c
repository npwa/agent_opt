#include <stdlib.h>
#include "ringbuf.h"
struct ringbuf { uint8_t *buf; size_t cap, head, size; };
ringbuf_t *rb_create(size_t c){ if(!c) return NULL; ringbuf_t *r=malloc(sizeof *r); if(!r) return NULL; r->buf=malloc(c); if(!r->buf){free(r);return NULL;} r->cap=c;r->head=r->size=0; return r;}
void rb_destroy(ringbuf_t *r){ if(r){free(r->buf);free(r);} }
size_t rb_write(ringbuf_t *r,const uint8_t*d,size_t n){ if(!r||!d) return 0; size_t f=r->cap-r->size; if(n>f)n=f; for(size_t i=0;i<n;i++) r->buf[(r->head+r->size+i)%r->cap]=d[i]; r->size+=n; return n;}
size_t rb_peek(const ringbuf_t*r,uint8_t*o,size_t n){ if(!r||!o) return 0; if(n>r->size)n=r->size; for(size_t i=0;i<n;i++) o[i]=r->buf[(r->head+i)%r->cap]; return n;}
size_t rb_read(ringbuf_t*r,uint8_t*o,size_t n){ n=rb_peek(r,o,n); if(r){r->head=(r->head+n)%r->cap; r->size-=n;} return n;}
size_t rb_size(const ringbuf_t*r){return r->size;}
size_t rb_capacity(const ringbuf_t*r){return r->cap;}
void rb_clear(ringbuf_t*r){ if(r){r->head=r->size=0;} }
