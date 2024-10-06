#ifndef BITMAPFROMMEM_H
#define BITMAPFROMMEM_H

#include <wx/bmpbndl.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

#define bitmapBundleFromSVGResource(name) wxBitmapBundle::FromSVGFile(wxFileName(wxStandardPaths::Get().GetResourcesDir(), name, "svg").GetFullPath(), wxSize(24,24))

#endif

