/// \file
/// \ingroup tutorial_webgui
///
/// Example showing RBrowser, web-based ROOT files and objects browser.
/// \macro_code
///
/// \date 2019-05-29
/// \authors Bertrand Bellenot <Bertrand.Bellenot@cern.ch>, Sergey Linev <S.Linev@gsi.de>

/*************************************************************************
 * Copyright (C) 1995-2019, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

// macro must be here to let macro work on Windows
R__LOAD_LIBRARY(libROOTBrowserv7)

#include <ROOT/RBrowser.hxx>

void browser()
{
   // create browser
   auto br = std::make_shared<ROOT::RBrowser>();

   // clear when connection to client closed
   br->ClearOnClose(br);
}
