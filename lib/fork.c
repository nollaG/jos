// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800
extern void _pgfault_upcall(void);

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
  addr = ROUNDDOWN(addr,PGSIZE);
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at vpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
  if (!(err & FEC_WR))
    panic("not a write");
  pte_t pte = vpt[PGNUM(addr)];
  if (!(pte & PTE_P) || !(pte & PTE_COW))
    panic("not PTE_COW");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.

	// LAB 4: Your code here.
  if ((r=sys_page_alloc(0,PFTEMP,PTE_P|PTE_U|PTE_W)) < 0)
    panic("sys_page_alloc:%e",r);
  memmove(PFTEMP,addr,PGSIZE);
  if ((r=sys_page_map(0,PFTEMP,0,addr,PTE_P|PTE_U|PTE_W))<0)
    panic("sys_page_map:%e",r);
  if ((r=sys_page_unmap(0,PFTEMP)) < 0)
    panic("sys_page_unmap:%e",r);
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;
  pte_t pte = vpt[pn];
  int perm = pte & PTE_SYSCALL;
  void* addr = (void*)(pn*PGSIZE);

  if (perm & (PTE_W | PTE_COW)) {
    perm = (perm | PTE_COW) & ~PTE_W;
    if ((r=sys_page_map(0,addr,envid,addr,perm)) < 0)
      panic("sys_page_map:%e",r);
    if ((r=sys_page_map(0,addr,0,addr,perm))<0)
      panic("sys_page_map:%e",r);
  } else {
    if ((r=sys_page_map(0,addr,envid,addr,PTE_U|PTE_P)) < 0) //read-only
      panic("sys_page_map:%e",r);
  }

	return 0;
}

static int
duppage_sfork(envid_t envid,unsigned pn) {
	int r;
  pte_t pte = vpt[pn];
  int perm = pte & PTE_SYSCALL;
  void* addr = (void*)(pn*PGSIZE);
  if ((r=sys_page_map(0,addr,envid,addr,perm)) < 0)
    panic("sys_page_map:%e",r);
  return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use vpd, vpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
  envid_t envid;
  uint32_t addr;
  int r;

  set_pgfault_handler(pgfault);
  envid = sys_exofork();
	if (envid < 0)
		panic("sys_exofork: %e", envid);

	if (envid == 0) {
		return 0;
	}


  for (addr = (uint32_t) UTEXT; addr < UTOP-PGSIZE ; addr+=PGSIZE) {
    if ((vpd[PDX(addr)] & PTE_P) //have this page
        && (vpt[PGNUM(addr)] & PTE_P)
        && (vpt[PGNUM(addr)] & PTE_U))
      duppage(envid,PGNUM(addr));
  }
  if ((r=sys_page_alloc(envid,(void*)(UXSTACKTOP-PGSIZE),PTE_P|PTE_U|PTE_W))<0) //alloc the exception stack
    panic("sys_page_alloc:%e",r);
  sys_env_set_pgfault_upcall(envid,_pgfault_upcall);
	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e", r);
  return envid;
}

// Challenge!
int
sfork(void)
{
  envid_t envid;
  uint32_t addr;
  int r;
  set_pgfault_handler(pgfault);
  envid = sys_exofork();
	if (envid < 0)
		panic("sys_exofork: %e", envid);

	if (envid == 0) {
		return 0;
	}
  for (addr = (uint32_t) UTEXT; addr < USTACKTOP-PGSIZE ; addr+=PGSIZE) {
    if ((vpd[PDX(addr)] & PTE_P) //have this page
        && (vpt[PGNUM(addr)] & PTE_P)
        && (vpt[PGNUM(addr)] & PTE_U))
      duppage_sfork(envid,PGNUM(addr));
  }
  duppage(envid,PGNUM(addr)); //dup the stack
  if ((r=sys_page_alloc(envid,(void*)(UXSTACKTOP-PGSIZE),PTE_P|PTE_U|PTE_W))<0) //alloc the exception stack
    panic("sys_page_alloc:%e",r);
  sys_env_set_pgfault_upcall(envid,_pgfault_upcall);
	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e", r);
  return envid;
}
