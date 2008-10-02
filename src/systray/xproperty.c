#include "xproperty.h"
#include "docker.h"

gboolean xprop_get8(Window window, Atom atom, Atom type, int size,
                    gulong *count, guchar **value)
{
  Atom ret_type;
  int ret_size;
  unsigned long ret_bytes;
  int result;
  unsigned long nelements = *count;
  unsigned long maxread = nelements;

  *value = NULL;
  
  /* try get the first element */
  result = XGetWindowProperty(display, window, atom, 0l, 1l, False,
                              AnyPropertyType, &ret_type, &ret_size,
                              &nelements, &ret_bytes, value);
  if (! (result == Success && ret_type == type &&
         ret_size == size && nelements > 0)) {
    if (*value) XFree(*value);
    *value = NULL;
    nelements = 0;
  } else {
    /* we didn't the whole property's value, more to get */
    if (! (ret_bytes == 0 || maxread <= nelements)) {
      int remain;
      
      /* get the entire property since it is larger than one element long */
      XFree(*value);
      /*
        the number of longs that need to be retreived to get the property's
        entire value. The last + 1 is the first long that we retrieved above.
      */
      remain = (ret_bytes - 1)/sizeof(long) + 1 + 1;
      /* dont get more than the max */
      if (remain > size/8 * (signed)maxread)
        remain = size/8 * (signed)maxread;
      result = XGetWindowProperty(display, window, atom, 0l, remain,
                                  False, type, &ret_type, &ret_size,
                                  &nelements, &ret_bytes, value);
      /*
       If the property has changed type/size, or has grown since our first
        read of it, then stop here and try again. If it shrank, then this will
        still work.
      */
      if (!(result == Success && ret_type == type &&
            ret_size == size && ret_bytes == 0)) {
        if (*value) XFree(*value);
        xprop_get8(window, atom, type, size, count, value);
      }
    }
  }

  *count = nelements;
  return *value != NULL;
}

gboolean xprop_get32(Window window, Atom atom, Atom type, int size,
                     gulong *count, gulong **value)
{
  return xprop_get8(window, atom, type, size, count, (guchar**)value);
}
