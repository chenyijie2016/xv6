1200 #include "types.h"
1201 #include "defs.h"
1202 #include "param.h"
1203 #include "memlayout.h"
1204 #include "mmu.h"
1205 #include "proc.h"
1206 #include "x86.h"
1207 #include <stdio.h>
1208 
1209 #define CRTPORT 0x3d4
1210 static ushort *crt = (ushort*)P2V(0xb8000);  // CGA memory
1211 
1212 static void startothers(void);
1213 static void mpmain(void)  __attribute__((noreturn));
1214 extern pde_t *kpgdir;
1215 extern char end[]; // first address after kernel loaded from ELF file
1216 
1217 // Bootstrap processor starts running C code here.
1218 // Allocate a real stack and switch to it, first
1219 // doing some setup required for memory allocator to work.
1220 int
1221 main(void)
1222 {
1223   kinit1(end, P2V(4*1024*1024)); // phys page allocator
1224   kvmalloc();      // kernel page table
1225   mpinit();        // detect other processors
1226   lapicinit();     // interrupt controller
1227   seginit();       // segment descriptors
1228   picinit();       // disable pic
1229   ioapicinit();    // another interrupt controller
1230   consoleinit();   // console hardware
1231   uartinit();      // serial port
1232   pinit();         // process table
1233   tvinit();        // trap vectors
1234   binit();         // buffer cache
1235   fileinit();      // file table
1236   ideinit();       // disk
1237   startothers();   // start other processors
1238   kinit2(P2V(4*1024*1024), P2V(PHYSTOP)); // must come after startothers()
1239   userinit();      // first user process
1240   mpmain();        // finish this processor's setup
1241 }
1242 
1243 
1244 
1245 
1246 
1247 
1248 
1249 
1250 // Other CPUs jump here from entryother.S.
1251 static void
1252 mpenter(void)
1253 {
1254   switchkvm();
1255   seginit();
1256   lapicinit();
1257   mpmain();
1258 }
1259 
1260 static void print_hello(){
1261   int pos;
1262   // Cursor position: col + 80*row.
1263   outb(CRTPORT, 14);
1264   pos = inb(CRTPORT+1) << 8;
1265   outb(CRTPORT, 15);
1266   pos |= inb(CRTPORT+1);
1267   char* s = "hello";
1268   int i,j,k;
1269   for(i = 0; i < 15; i++){
1270     for(j = 0; j < 15; j++){
1271       for(k = 0; k < 5; k++){
1272         int c = s[k];
1273         int shift = (i<<12) | (j<<8);
1274         crt[pos++] = (c&0xff) | shift;  // black on white
1275         if((pos/80) >= 24){  // Scroll up.
1276           memmove(crt, crt+80, sizeof(crt[0])*23*80);
1277           pos -= 80;
1278           memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
1279         }
1280         outb(CRTPORT, 14);
1281         outb(CRTPORT+1, pos>>8);
1282         outb(CRTPORT, 15);
1283         outb(CRTPORT+1, pos);
1284         crt[pos] = ' ' | 0x0700;
1285       }
1286     }
1287   }
1288 }
1289 
1290 // Common CPU setup code.
1291 static void
1292 mpmain(void)
1293 {
1294   cprintf("cpu%d: starting %d\n", cpuid(), cpuid());
1295   idtinit();       // load idt register
1296   print_hello();
1297   xchg(&(mycpu()->started), 1); // tell startothers() we're up
1298   scheduler();     // start running processes
1299 }
1300 pde_t entrypgdir[];  // For entry.S
1301 
1302 // Start the non-boot (AP) processors.
1303 static void
1304 startothers(void)
1305 {
1306   extern uchar _binary_entryother_start[], _binary_entryother_size[];
1307   uchar *code;
1308   struct cpu *c;
1309   char *stack;
1310 
1311   // Write entry code to unused memory at 0x7000.
1312   // The linker has placed the image of entryother.S in
1313   // _binary_entryother_start.
1314   code = P2V(0x7000);
1315   memmove(code, _binary_entryother_start, (uint)_binary_entryother_size);
1316 
1317   for(c = cpus; c < cpus+ncpu; c++){
1318     if(c == mycpu())  // We've started already.
1319       continue;
1320 
1321     // Tell entryother.S what stack to use, where to enter, and what
1322     // pgdir to use. We cannot use kpgdir yet, because the AP processor
1323     // is running in low  memory, so we use entrypgdir for the APs too.
1324     stack = kalloc();
1325     *(void**)(code-4) = stack + KSTACKSIZE;
1326     *(void**)(code-8) = mpenter;
1327     *(int**)(code-12) = (void *) V2P(entrypgdir);
1328 
1329     lapicstartap(c->apicid, V2P(code));
1330 
1331     // wait for cpu to finish mpmain()
1332     while(c->started == 0)
1333       ;
1334   }
1335 }
1336 
1337 // The boot page table used in entry.S and entryother.S.
1338 // Page directories (and page tables) must start on page boundaries,
1339 // hence the __aligned__ attribute.
1340 // PTE_PS in a page directory entry enables 4Mbyte pages.
1341 
1342 __attribute__((__aligned__(PGSIZE)))
1343 pde_t entrypgdir[NPDENTRIES] = {
1344   // Map VA's [0, 4MB) to PA's [0, 4MB)
1345   [0] = (0) | PTE_P | PTE_W | PTE_PS,
1346   // Map VA's [KERNBASE, KERNBASE+4MB) to PA's [0, 4MB)
1347   [KERNBASE>>PDXSHIFT] = (0) | PTE_P | PTE_W | PTE_PS,
1348 };
1349 
1350 
1351 // Blank page.
1352 
1353 
1354 
1355 
1356 
1357 
1358 
1359 
1360 
1361 
1362 
1363 
1364 
1365 
1366 
1367 
1368 
1369 
1370 
1371 
1372 
1373 
1374 
1375 
1376 
1377 
1378 
1379 
1380 
1381 
1382 
1383 
1384 
1385 
1386 
1387 
1388 
1389 
1390 
1391 
1392 
1393 
1394 
1395 
1396 
1397 
1398 
1399 
1400 
1401 // Blank page.
1402 
1403 
1404 
1405 
1406 
1407 
1408 
1409 
1410 
1411 
1412 
1413 
1414 
1415 
1416 
1417 
1418 
1419 
1420 
1421 
1422 
1423 
1424 
1425 
1426 
1427 
1428 
1429 
1430 
1431 
1432 
1433 
1434 
1435 
1436 
1437 
1438 
1439 
1440 
1441 
1442 
1443 
1444 
1445 
1446 
1447 
1448 
1449 
1450 
1451 // Blank page.
1452 
1453 
1454 
1455 
1456 
1457 
1458 
1459 
1460 
1461 
1462 
1463 
1464 
1465 
1466 
1467 
1468 
1469 
1470 
1471 
1472 
1473 
1474 
1475 
1476 
1477 
1478 
1479 
1480 
1481 
1482 
1483 
1484 
1485 
1486 
1487 
1488 
1489 
1490 
1491 
1492 
1493 
1494 
1495 
1496 
1497 
1498 
1499 
