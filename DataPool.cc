#include "DataPool.h"

namespace Attic {

DataPool::~DataPool()
{
  if (AllChanges)
    delete AllChanges;
    
  for (std::vector<Location *>::iterator i = Locations.begin();
       i != Locations.end();
       i++)
    delete *i;
}

void DataPool::ComputeChanges()
{
  if (AllChanges)
    delete AllChanges;
  AllChanges = new ChangeSet;

  for (std::vector<Location *>::iterator i = Locations.begin();
       i != Locations.end();
       i++)
    if (*i != CommonAncestor && (*i)->PreserveChanges)
      AllChanges->CompareLocations(*i, CommonAncestor);
}

void DataPool::ResolveConflicts()
{
  for (ChangeSet::ChangesMap::iterator j = AllChanges->Changes.begin();
       j != AllChanges->Changes.end();
       j++)
    if ((*j).second->Next)
      std::cout << "There are conflicts!" << std::endl;
}

void DataPool::ApplyChanges(MessageLog& log)
{
  if (! AllChanges)
    return;

  // jww (2006-11-11): Move this logic in ChangeSet.cc
  ChangeSet::ChangesArray changesArray;

  for (ChangeSet::ChangesMap::iterator i = AllChanges->Changes.begin();
       i != AllChanges->Changes.end();
       i++)
    changesArray.push_back((*i).second);

  std::stable_sort(changesArray.begin(), changesArray.end(),
		   ChangeSet::ChangeComparer());

  for (std::vector<Location *>::iterator i = Locations.begin();
       i != Locations.end();
       i++) {
    ChangeSet::ChangesArray thisChangesArray;

    for (ChangeSet::ChangesArray::iterator j = changesArray.begin();
	 j != changesArray.end();
	 j++) {
      // Ignore changes in the origin's own repository (since the
      // change is already extant there).
      if (*i == (*j)->Item->Repository)
	continue;

      if ((*j)->ChangeKind == StateChange::Add) {
	// jww (2006-11-19): This gets complicated if the remote
	// location has changes.  We need to check the common ancestor
	// first, then see if any of those entries have been deleted
	// or changed by the remote site.  Also, we need to check if
	// files have been created at the remote site which are now
	// equivalent.
	(*j)->Duplicates = CommonAncestor->ExistsAtLocation((*j)->Item);
	if ((*j)->Duplicates) {
	  thisChangesArray.push_front(*j);
	  continue;
	}
      }
      thisChangesArray.push_back(*j);
    }
	
    for (ChangeSet::ChangesArray::iterator j = thisChangesArray.begin();
	 j != thisChangesArray.end();
	 j++)
      for (StateChange * ptr = *j; ptr; ptr = ptr->Next)
	if (LoggingOnly) {
	  ptr->Report(log);
	} else {
	  ChangeSet * changeSet = (*i)->CurrentChanges;
	  if (! changeSet && ! (*i)->PreserveChanges)
	    changeSet = AllChanges;
	  (*i)->ApplyChange(&log, *ptr, *changeSet);
	}
  }
}

} // namespace Attic
