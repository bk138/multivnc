#ifndef BITMAPFROMMEM_H
#define BITMAPFROMMEM_H

#include <wx/mstream.h>

#define bitmapFromMem(name) _GetBitmapFromMemory(name, sizeof(name))

inline wxBitmap _GetBitmapFromMemory(const unsigned char *data, int length) 
{
   wxMemoryInputStream is(data, length);
   return wxBitmap(wxImage(is, wxBITMAP_TYPE_ANY, -1), -1);
}


#endif

