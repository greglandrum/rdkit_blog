Optimizing cleanup of stereochemistry
-------------------------------------

A fair amount of the time spent in constructing molecules using the
RDKit goes to the assignment/cleanup of stereochemistry. I did an
analysis for the 2012 UGM where I found that 27% of the time spent
parsing 100K drug-like molecules from the ZINC set was in the
``assignStereochemistry()`` function. The function itself is responsible
for assigning R/S labels to atoms and, more importantly, removing
stereochemistry flags from atoms or double bonds that shouldn't have
them (where the substituents are the same).

I think the assignStereochemistry() step is important (to be sure that
molecules are actually correctly specified), but it certainly wouldn't
be bad to make it faster. That's the point of this exercise.

As a readily availblbe test set for this, I'm going to take the ChEMBL
molecules that appeared in documents published betwen 2010 and 2012
that I used in a previous blog post:
http://rdkit.blogspot.ch/2013/12/finding-related-documents-in-chembl-2.html. The
full set here contains 234681 molecules, since I'm only interested in
molecules that have stereochemistry, I narrowed the set down like
this::
  
   egrep '@|/' chembl16_2010-2012.smi > chembl16_2010-2012.chiral.smi
  
That gives 81030 molecules that contain at least one specified chiral
center or double bond with specified stereochemistry.
For the purposes of this exercise, where I don't want to wait forever,
I used the first 40K of those.

As with any optimization exercise, I did this one using the profiler
and a relatively simple bit of code that reads in the 40K molecules
and then generates canonical smiles for them. The SMILES generation
isn't actually important here, but it does provide some useful
information about relative timings.

+---------------------+-------+-------+
|Operation            |Percent|Time(s)|
+---------------------+-------+-------+
| Total               |100.0  |30.6   |
+---------------------+-------+-------+
|Sanitize             |26.0   |8.0    |
+---------------------+-------+-------+
|assignStereochemistry|24.5   |7.4    |
+---------------------+-------+-------+
|MolToSmiles          |39.9   |12.2   |
+---------------------+-------+-------+
|   rankAtoms         |21.1   |6.4    |
+---------------------+-------+-------+

The ``rankAtoms`` step is part of ``MolToSmiles``.

After an embarassing amount of time pursuing avenues that led to no
real improvement, I made a couple of small changes (commits https://github.com/rdkit/rdkit/commit/d779c850c9696948f8e718ad790e0224ea7320b8#diff-ca0dbad92a874b2f69b549293387925e
and https://github.com/rdkit/rdkit/commit/4d47482f0f9ac7d67be6a811232651da8e5dc635#diff-ca0dbad92a874b2f69b549293387925e)
that ended up helping a fair amount:

+---------------------+-------+-------+
|Operation            |Percent|Time(s)|
+---------------------+-------+-------+
| Total               |100.0  |28.8   |
+---------------------+-------+-------+
|Sanitize             |26.9   |7.7    |
+---------------------+-------+-------+
|assignStereochemistry|20.3   |5.8    |
+---------------------+-------+-------+
|MolToSmiles          |42.9   |12.3   |
+---------------------+-------+-------+
|   rankAtoms         |23.8   |6.8    |
+---------------------+-------+-------+

The ``rankAtoms`` step is part of ``MolToSmiles``.

                               
Stats about the dataset
=======================

* Number of compounds considered: 40000
* Number where stereochemistry was resolved in one pass: 38282
* Number where dependent stereochemistry required two passes: 1712
* Number where dependent stereochemistry required three passes: 6

Molecules where three passes were required
++++++++++++++++++++++++++++++++++++++++++

* CN(C)C(=O)N[C@@H]1CC[C@@H](CN2[C@@H]3CC[C@H]2C[C@H](C3)Oc4cccc(c4)C(=O)N)CC1
* CN(C)C(=O)N[C@@H]1CC[C@@H](CCN2[C@@H]3CC[C@H]2C[C@H](C3)Oc4cccc(c4)C(=O)N)CC1
* CN(C)C(=O)N[C@@H]1CC[C@H](CN2[C@@H]3CC[C@H]2C[C@H](C3)Oc4cccc(c4)C(=O)N)CC1
* CN(C)C(=O)N[C@@H]1CC[C@H](CCN2[C@@H]3CC[C@H]2C[C@H](C3)Oc4cccc(c4)C(=O)N)CC1
* Cc1ccc(s1)C(=CCCN2C[C@@H]3[C@H](C2)[C@@H]3C(=O)O)c4ccc(C)s4
* Cc1ccc(s1)C(=CCCN2C[C@@H]3[C@H](C2)[C@H]3C(=O)O)c4ccc(C)s4

Those all look reasonable.

