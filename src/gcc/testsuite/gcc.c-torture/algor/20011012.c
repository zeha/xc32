/* reproduce customer loop optimisation bug */

#define NULL (void *)0

typedef unsigned int uint32;

typedef struct
{
  uint32                startOfFreeList;
  uint32                elementSize;
  uint32                numOfElements;
  uint32                numOfFreeElements;
  uint32                nextPageAddress;
  uint32                headerSize;
  int		        memClientPoolID;
  uint32                allocationID;  
  int		        lifeSpan;
  void  		*ptr2MemControlType;
} HeaderType;

int LinkNewPage (HeaderType* ptr2GroupIDHeader, 
		   uint32 address2NextPage)
{
     uint32  index                           = 0;
     uint32  elementSize                     = 0;
     uint32  headerSize                      = 0;
     uint32  numOfElementsPerPage            = 0;
     uint32  currentAddress                  = 0;
     uint32  lastAddress                     = 0;
     uint32  nextAddress                     = 0;
     uint32  previousAddress                 = 0;
     uint32  nextPagesFreeListStart          = 0;
     uint32  allocID                         = 1;
     
     int  whichPool             	     = 10;

     HeaderType *previousPagePtr2Header   = NULL;
     HeaderType *nextPagePtr2Header       = NULL;
     int         lifeSpan                 = 12;
     void       *ptr2MemControlType       = 0;

     previousPagePtr2Header =  ptr2GroupIDHeader;
     while (previousPagePtr2Header->nextPageAddress != (uint32)0)
     {
         previousPagePtr2Header =
                   (HeaderType*)previousPagePtr2Header->nextPageAddress;
     }

     nextPagePtr2Header   = (HeaderType*)address2NextPage;
     elementSize          = ptr2GroupIDHeader->elementSize;
     numOfElementsPerPage = ptr2GroupIDHeader->numOfElements;
     headerSize           = ptr2GroupIDHeader->headerSize;
     whichPool            = ptr2GroupIDHeader->memClientPoolID;
     allocID              = ptr2GroupIDHeader->allocationID; 
     lifeSpan             = ptr2GroupIDHeader->lifeSpan; 
     ptr2MemControlType   = ptr2GroupIDHeader->ptr2MemControlType;

     previousPagePtr2Header->nextPageAddress = address2NextPage;
     nextPagesFreeListStart   = address2NextPage + headerSize;
     nextPagePtr2Header = (HeaderType*)address2NextPage;
     nextPagePtr2Header->startOfFreeList     = nextPagesFreeListStart;
     nextPagePtr2Header->elementSize         = elementSize;
     nextPagePtr2Header->numOfElements       = numOfElementsPerPage;
     nextPagePtr2Header->numOfFreeElements   = numOfElementsPerPage;
     nextPagePtr2Header->nextPageAddress     = 0;
     nextPagePtr2Header->headerSize          = headerSize;
     nextPagePtr2Header->memClientPoolID     = whichPool;
     nextPagePtr2Header->allocationID        = allocID;
     nextPagePtr2Header->lifeSpan            = lifeSpan;
     nextPagePtr2Header->ptr2MemControlType  = ptr2MemControlType;

     /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
     /*CREATE THE LINKED FREE LIST                                   */
     /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
     for (index = 0, currentAddress = previousAddress = nextPagesFreeListStart;
          index < ( numOfElementsPerPage ) ;
          index++)
     {
          nextAddress = currentAddress + elementSize;
          *(uint32*)currentAddress = nextAddress;
          previousAddress = currentAddress;
          currentAddress = nextAddress;
     }

     /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
     /*FINISH THE LIST BY TERMINATING THE LAST ONE                         */
     /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
     lastAddress = previousAddress;
     *(uint32*)lastAddress = 0;

     return (1);
}

#define PGSIZE	8192
#define ELSIZE	64
double headpage[PGSIZE/sizeof(double)];
double newpage[PGSIZE/sizeof(double)];

#define ALSZ	sizeof(double)

int
main ()
{
     HeaderType *head = (HeaderType *)headpage;
     HeaderType *new = (HeaderType *)newpage;
     uint32 index;
     char *elt;

     /* initialise group header */
     head->nextPageAddress	= 0;
     head->elementSize		= ELSIZE;
     head->headerSize		= (sizeof (HeaderType) + ALSZ - 1) & ~(ALSZ - 1);
     head->numOfElements	= (PGSIZE - head->headerSize) / ELSIZE;
     head->memClientPoolID	= 1;
     head->allocationID		= 2; 
     head->lifeSpan		= 3; 
     head->ptr2MemControlType	= (void *)4;

     /* link in new page */
     LinkNewPage (head, (uint32)newpage);

     /* check chunk free count */
     if (new->numOfFreeElements != head->numOfElements
         || new->elementSize != ELSIZE)
       abort ();

     /* check chunk free list */
     elt = new->startOfFreeList;
     for (index = new->numOfElements - 1; index != 0; index--) 
       {
	 /* first word of each chunk should point at next chunk */
	 if (*(uint32 *)elt != elt + ELSIZE)
	   abort ();
	 elt += ELSIZE;
       }

     /* except last chunk which should have null pointer */
     if (*(uint32 *)elt != 0)
       abort ();
     
     /* all checks out */
     exit (0);
}
