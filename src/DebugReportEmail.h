
#ifndef DEBUG_REPORT_EMAIL_H
#define DEBUG_REPORT_EMAIL_H

#include <wx/debugrpt.h>

/**
   DebugReportEmail is a wxDebugReport which opens the user's default mail
   user agent with the report's files in the message body.
 */
class DebugReportEmail: public wxDebugReport
{
 public:
   DebugReportEmail(const wxString &toEmail, const wxString &subject)
       : m_toEmail(toEmail), m_subject(subject) {};
 protected:
    virtual bool DoProcess() override;
 private:
    wxString m_toEmail, m_subject;
};

#endif
