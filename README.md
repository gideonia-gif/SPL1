# SPL1 – Assignment 5: Bank Management System (Java)

## Layout
```
src/          all .java files (skeletons from VPL + given files from the Moodle zip)
bin/          compiled classes (ignored by git)
run.sh        compile everything, optionally run a class
.devcontainer Docker environment for VS Code (Java 17)
```

## First-time setup
1. Install the VS Code extension **Dev Containers** (ms-vscode-remote.remote-containers).
2. Make sure Docker Desktop is running.
3. In VS Code: `File > Open Folder…` → this folder → click **Reopen in Container**
   (or Cmd+Shift+P → "Dev Containers: Reopen in Container"). First build takes a few minutes.
4. Put the files in `src/`:
   - Skeleton files from VPL: BankAccount, AccountComparatorByName, AccountComparatorByNumber,
     BinaryNode, TestToString, BankAccountsBinarySearchTree, TestBalance, FilterByBalance,
     FilterByAccountNumber, FilterIterator, BankAccountFiltering, Bank
   - Everything from the Moodle zip (Stack, List, DynamicArray, LinkedList, Link, BinaryTree,
     BinarySearchTree, BinarySearchNode, BinaryTreeInOrderIterator, Filter, ...) – do NOT modify these.
5. Check it compiles: `./run.sh`  then e.g. `./run.sh TestToString`

## Git
```
git add .
git commit -m "message"
git push
```
