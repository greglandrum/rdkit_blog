//
//  Copyright (C) 2021 Greg Landrum
//

#include <GraphMol/FileParsers/MolSupplier.h>
#include <GraphMol/MolHash/MolHash.h>
#include <GraphMol/RDKitBase.h>
#include <RDGeneral/RDLog.h>
#include <algorithm>
#include <boost/timer/timer.hpp>
#include <iostream>
#include <vector>

using namespace RDKit;

void readmols(std::string pathName, unsigned int maxToDo,
              std::vector<RWMOL_SPTR> &mols) {
  boost::timer::auto_cpu_timer t;
  // using a supplier without sanitizing the molecules...
  RDKit::SmilesMolSupplier suppl(pathName, " \t", 1, 0, true, false);
  unsigned int nDone = 0;
  while (!suppl.atEnd() && (maxToDo <= 0 || nDone < maxToDo)) {
    RDKit::ROMol *m = suppl.next();
    if (!m) {
      continue;
    }
    m->updatePropertyCache();
    // the tautomer hash code uses conjugation info
    MolOps::setConjugation(*m);
    nDone += 1;
    mols.push_back(RWMOL_SPTR((RWMol *)m));
  }
  std::cerr << "read: " << nDone << " mols." << std::endl;
}

void generatehashes(const std::vector<RWMOL_SPTR> &mols) {
  boost::timer::auto_cpu_timer t;
  for (auto &mol : mols) {
    auto hash =
        MolHash::MolHash(mol.get(), MolHash::HashFunction::HetAtomTautomer);
  }
}
int main(int argc, char *argv[]) {
  RDLog::InitLogs();
  std::vector<RWMOL_SPTR> mols;
  BOOST_LOG(rdInfoLog) << "read mols" << std::endl;

  readmols(argv[1], 10000, mols);
  BOOST_LOG(rdInfoLog) << "generate hashes" << std::endl;
  generatehashes(mols);

  BOOST_LOG(rdInfoLog) << "done " << std::endl;
}
