
#include "DebugReportEmail.h"

#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>
#include <wx/wx.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>

using namespace std;

// https://stackoverflow.com/a/17708801/361413
// TODO convert to wx-only
static string url_encode(const string &value) {
    ostringstream escaped;
    escaped.fill('0');
    escaped << hex;

    for (string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        string::value_type c = (*i);

        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << uppercase;
        escaped << '%' << setw(2) << int((unsigned char) c);
        escaped << nouppercase;
    }

    return escaped.str();
}

bool DebugReportEmail::DoProcess()
{
    wxString body;
    for (size_t i = GetFilesCount()-1; ; --i) {
	// read in each file's contents
	wxString fileName;
	GetFile(i, &fileName, NULL);
	wxString filePath = GetDirectory() + wxFILE_SEP_PATH + fileName;
	wxString fileContent;
	wxFileInputStream input(filePath);
	wxTextInputStream text(input, wxT(" \t"), wxConvUTF8);
	while (input.IsOk() && !input.Eof()) {
	    fileContent += text.ReadLine() + "\n";
	}

	// append to message body string
	body += "-------" + fileName + "-------\n" + fileContent + "\n";

	// can't use for loop's check as size_t wraps around
	if(i == 0)
	    break;
    }

    return wxLaunchDefaultBrowser("mailto:" + m_toEmail +
				  "?subject=" + url_encode(m_subject.ToStdString()) +
				  "&body=" + url_encode(body.ToStdString()));
}

