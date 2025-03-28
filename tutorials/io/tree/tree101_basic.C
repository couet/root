/// \file
/// \ingroup tutorial_tree
/// \notebook -nodraw
///  Read data from an ascii file and create a root file with an histogram and an ntuple.
///  See a variant of this macro in tree102_basic.C.
///
/// \macro_code
///
/// \author Rene Brun

#include "Riostream.h"

void tree101_basic()
{
   // read file $ROOTSYS/tutorials/io/tree/basic.dat
   // this file has 3 columns of float data
   TString dir = gROOT->GetTutorialDir();
   dir.Append("/io/tree/");
   dir.ReplaceAll("/./", "/");
   ifstream in;
   in.open(TString::Format("%sbasic.dat", dir.Data()));

   Float_t x, y, z;
   Int_t nlines = 0;
   auto f = TFile::Open("tree101.root", "RECREATE");
   TH1F h1("h1", "x distribution", 100, -4, 4);
   TNtuple ntuple("ntuple","data from ascii file", "x:y:z");

   while (1) {
      in >> x >> y >> z;
      if (!in.good())
         break;
      if (nlines < 5)
         printf("x = %+8.6f, y = %+8.6f, z = %+8.6f\n", x, y, z);
      h1.Fill(x);
      ntuple.Fill(x, y, z);
      nlines++;
   }
   printf(" found %d points\n", nlines);
   in.close();
   f->Write();
}
