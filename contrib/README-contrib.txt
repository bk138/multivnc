
WinVNC.exe and VNCHooks.dll were built from windows source 1.3.9
downloaded from http://www.tightvnc.com/download.html in January 2009.

The only (quick&dirty) changes made are in winvnc/WinVNC.cpp.
They allow starting TightVNC with options instead of 
needing to attach to an already running instance.


Christian Beier <dontmind@freeshell.org>



-----------snip----------
--- WinVNC.cpp.orig	2006-12-05 10:35:45.000000000 +0100
+++ WinVNC.cpp	2009-01-16 16:43:28.000000000 +0100
@@ -54,6 +54,12 @@
 
 DWORD		mainthreadId;
 
+bool        bOneshot;
+HWND        hShareWindow;
+bool		cancelConnect;
+char        *connectName;
+int         connectPort;
+
 // WinMain parses the command line and either calls the main App
 // routine or, under NT, the main service routine.
 int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
@@ -92,9 +98,9 @@
 	}
 
 	BOOL argfound = FALSE;
-	char *connectName = NULL;
-	int connectPort;
-	bool cancelConnect = false;
+	
+	
+	
 
 	for (i = 0; i < strlen(szCmdLine); i++)
 	{
@@ -117,6 +123,14 @@
 			vncService::PostUserHelperMessage();
 			return 0;
 		}
+		if (strncmp(&szCmdLine[i], winvncRunAsUserAppOneshot, arglen) == 0 &&
+			arglen == strlen(winvncRunAsUserAppOneshot))
+		{
+			// WinVNC is being run in one-shot mode
+			bOneshot = true;
+			i += arglen;
+			continue;
+		}
 		if (strncmp(&szCmdLine[i], winvncRunService, arglen) == 0 &&
 			arglen == strlen(winvncRunService))
 		{
@@ -278,7 +292,10 @@
 					HWND hwndFound = vncService::FindWindowByTitle(title);
 					if (hwndFound != NULL)
 						cancelConnect = false;
-					vncService::PostShareWindow(hwndFound);
+					if(bOneshot)
+						hShareWindow = hwndFound;
+					else
+						vncService::PostShareWindow(hwndFound);
 					delete [] title;
 				}
 			}
@@ -317,8 +334,11 @@
 		// Either the user gave the -help option or there is something odd on the cmd-line!
 
 		// Show the usage dialog
+		
+		
 		MessageBox(NULL, winvncUsageText, "WinVNC Usage", MB_OK | MB_ICONINFORMATION);
 		break;
+		
 	}
 
 	// If no arguments were given then just run
@@ -331,13 +351,18 @@
 			VCard32 address = VSocket::Resolve(connectName);
 			if (address != 0) {
 				// Post the IP address to the server
-				vncService::PostAddNewClient(address, connectPort);
+				if(!bOneshot)
+					vncService::PostAddNewClient(address, connectPort);
 			}
 		} else {
 			// Tell the server to show the Add New Client dialog
 			vncService::PostAddNewClient(0, 0);
 		}
 	}
+
+	if(bOneshot)
+		return WinVNCAppMain();
+
 	if (connectName != NULL)
 		delete[] connectName;
 
@@ -378,6 +403,24 @@
 		PostQuitMessage(0);
 	}
 
+	if(bOneshot)
+	{
+		if(hShareWindow != NULL)
+			vncService::PostShareWindow(hShareWindow);
+		
+		if (connectName != NULL && !cancelConnect) 
+		{
+			if (connectName[0] != '\0') 
+				{
+					VCard32 address = VSocket::Resolve(connectName);
+					if (address != 0) // Post the IP address to the server
+						vncService::PostAddNewClient(address, connectPort);
+			   }
+		}
+			
+
+	}
+
 	// Now enter the message handling loop until told to quit!
 	MSG msg;
 	while (GetMessage(&msg, NULL, 0,0) ) {

-------------snap--------------------------------