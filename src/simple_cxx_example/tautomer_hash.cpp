//
//  Copyright (C) 2021-2025 Greg Landrum
//

#include <GraphMol/FileParsers/MolSupplier.h>
#include <GraphMol/MolHash/MolHash.h>
#include <GraphMol/RDKitBase.h>
#include <algorithm>
#include <boost/timer/timer.hpp>
#include <iostream>
#include <vector>

using namespace RDKit;

void readmols(std::string pathName, unsigned int maxToDo,
              std::vector<std::unique_ptr<RWMol>> &mols) {
  boost::timer::auto_cpu_timer t;
  // using a supplier without sanitizing the molecules...
  v2::FileParsers::SmilesMolSupplierParams params;
  params.parseParameters.sanitize = false;
  params.smilesColumn = 1;
  params.nameColumn = 0;
  v2::FileParsers::SmilesMolSupplier suppl(pathName, params);
  unsigned int nDone = 0;
  while (!suppl.atEnd() && (maxToDo <= 0 || nDone < maxToDo)) {
    auto m = suppl.next();
    if (!m) {
      continue;
    }
    m->updatePropertyCache();
    // the tautomer hash code uses conjugation info
    MolOps::setConjugation(*m);
    nDone += 1;
    mols.push_back(std::move(m));
  }
  std::cerr << "  read: " << nDone << " mols." << std::endl;
}

void generatehashes(const std::vector<std::unique_ptr<RWMol>> &mols) {
  boost::timer::auto_cpu_timer t;
  for (const auto &mol : mols) {
    auto hash =
        MolHash::MolHash(mol.get(), MolHash::HashFunction::HetAtomTautomer);
  }
}
int main(int argc, char *argv[]) {
  std::vector<std::unique_ptr<RWMol>> mols;
  std::cerr << "reading molecules" << std::endl;
  readmols(argv[1], 10000, mols);
  std::cerr << "generating hashes" << std::endl;
  generatehashes(mols);
  std::cerr << "done" << std::endl;
}
