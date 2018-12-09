# Boost-free code project
## Overview
This repository uses a very old version of boost.  This introduces a number of build complexities.  As part of our effort to bring the code up-to-speed with the current versions of the dependencies and STL, we are taking steps to minimize if not remove the use of boost libraries in favor of STL, and updating the use of boost to the latest version.

## Steps
Current way takes two stages.
- For each library and objects of boost, find a potential alternative in the STL.
- If the equivalent library exists in STL, replace the statement using the tools/boostrm.sh
    - boostrm.sh takes two arguments: name of the object in question, and STL library for the include statement.
    - boostrm.sh assumes boost and STL shares the same naming convntion.
- If everything worked, commit and PR.  If not, fix it and PR.

## If STL does not have equivalent library
- Bring up the information in the relevant issue and discuss how to tailor to make the substitution work.
- Impliment the customized approach and PR
- *or* -
- Create an issue to add the boost library to the modular install of the boost in the installation script.

## Thank you.
If you are interested in finding all files that contains a particular boost library, you can try:
```sh
git grep 'boost::weak_ptr' | sed -ne 's/^\([^:]*\):.*$/\1/p' | sort | uniq  

```
from a sh/bash/zsh/ksh environment.
