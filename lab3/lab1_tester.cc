/*
 *  You need not go through or edit this file to complete this lab.
 * 
 * */

/* 
 * By Yutao Liu
 * 
 * 	for CSP-2013-Fall
 * ---------------
 * 
 * By Zhe Qiu
 * 	
 * 	for CSE-2014-Undergraduate
 * 
 * -----------------
 * 
 * 
 * */


/* lab1 tester.  
 * Test whether extent_client -> extent_server -> inode_manager behave correctly
 */

#include "extent_client.h"
#include <stdio.h>

#define FILE_NUM 50
#define LARGE_FILE_SIZE 512*64

#define iprint(msg) \
    printf("[TEST_ERROR]: %s\n", msg);
extent_client *ec;
int total_score = 0;

int test_create_and_getattr()
{
    int i, rnum;
    extent_protocol::extentid_t id;
    extent_protocol::attr a;

    printf("========== begin test create and getattr ==========\n");

    srand((unsigned)time(NULL));

    for (i = 0; i < FILE_NUM; i++) {
        rnum = rand() % 10;
        memset(&a, 0, sizeof(a));
        if (rnum < 3) {
            ec->create(extent_protocol::T_DIR, id);
            if ((int)id == 0) {
                iprint("error creating dir\n");
                return 1;
            }
            if (ec->getattr(id, a) != extent_protocol::OK) {
                iprint("error getting attr, return not OK\n");
                return 2;
            }
            if (a.type != extent_protocol::T_DIR) {
                iprint("error getting attr, type is wrong");
                return 3;
            }
        } else {
            ec->create(extent_protocol::T_FILE, id);
            if ((int)id == 0) {
                iprint("error creating dir\n");
                return 1;
            }
            if (ec->getattr(id, a) != extent_protocol::OK) {
                iprint("error getting attr, return not OK\n");
                return 2;
            }
            if (a.type != extent_protocol::T_FILE) {
                iprint("error getting attr, type is wrong");
                return 3;
            }
        }
    } 
    total_score += 40; 
    printf("========== pass test create and getattr ==========\n");
    return 0;
}

int test_indirect()
{
    int i, rnum;
    char *temp1 = (char *)malloc(LARGE_FILE_SIZE);
    char *temp2 = (char *)malloc(LARGE_FILE_SIZE);
    extent_protocol::extentid_t id1, id2;
    printf("begin test indirect\n");
    srand((unsigned)time(NULL));
    
    ec->create(extent_protocol::T_FILE, id1);
    for (i = 0; i < LARGE_FILE_SIZE; i++) {
        rnum = rand() % 26;
        temp1[i] = 97 + rnum;
    }
    std::string buf(temp1);
    if (ec->put(id1, buf) != extent_protocol::OK) {
        printf("error put, return not OK\n");
        return 1;
    }
    ec->create(extent_protocol::T_FILE, id2);
    for (i = 0; i < LARGE_FILE_SIZE; i++) {
        rnum = rand() % 26;
        temp2[i] = 97 + rnum;
    }
    std::string buf2(temp2);
    if (ec->put(id2, buf2) != extent_protocol::OK) {
        printf("error put, return not OK\n");
        return 2;
    }
    std::string buf_2;
    if (ec->get(id1, buf_2) != extent_protocol::OK) {
        printf("error get, return not OK\n");
        return 3;
    }
    if (buf.compare(buf_2) != 0) {
        std::cout << "error get large file, not consistent with put large file : " << 
            buf << " <-> " << buf_2 << "\n";
        return 4;
    }
    std::string buf2_2;
    if (ec->get(id2, buf2_2) != extent_protocol::OK) {
        printf("error get, return not OK\n");
        return 5;
    }
    if (buf2.compare(buf2_2) != 0) {
        std::cout << "error get large file 2, not consistent with put large file : " << 
            buf2 << " <-> " << buf2_2 << "\n";
        return 6;
    }

    total_score += 10;
    printf("end test indirect\n");
    return 0;
}

int test_put_and_get()
{
    int i, rnum;
    extent_protocol::extentid_t id;
    extent_protocol::attr a;
    int contents[FILE_NUM];
    char *temp = (char *)malloc(10);

    printf("========== begin test put and get ==========\n");
    srand((unsigned)time(NULL));
    for (i = 0; i < FILE_NUM; i++) {
        memset(&a, 0, sizeof(a));
        id = (extent_protocol::extentid_t)(i+2);
        if (ec->getattr(id, a) != extent_protocol::OK) {
            iprint("error getting attr, return not OK\n");
            return 1;
        }
        if (a.type == extent_protocol::T_FILE) {
            rnum = rand() % 10000;
            memset(temp, 0, 10);
            sprintf(temp, "%d", rnum);
            std::string buf(temp);
            if (ec->put(id, buf) != extent_protocol::OK) {
                iprint("error put, return not OK\n");
                return 2;
            }
            contents[i] = rnum;
        }
    }
    for (i = 0; i < FILE_NUM; i++) {
        memset(&a, 0, sizeof(a));
        id = (extent_protocol::extentid_t)(i+2);
        if (ec->getattr(id, a) != extent_protocol::OK) {
            iprint("error getting attr, return not OK\n");
            return 3;
        }
        if (a.type == extent_protocol::T_FILE) {
            std::string buf;
            if (ec->get(id, buf) != extent_protocol::OK) {
                iprint("error get, return not OK\n");
                return 4;
            }
            memset(temp, 0, 10);
            sprintf(temp, "%d", contents[i]);
            std::string buf2(temp);
            if (buf.compare(buf2) != 0) {
                std::cout << "[TEST_ERROR] : error get, not consistent with put " << 
                    buf << " <-> " << buf2 << "\n\n";
                return 5;
            }
        }
    } 

    total_score += 30;
    test_indirect(); 
    printf("========== pass test put and get ==========\n");
    return 0;
}

int test_remove()
{
    int i;
    extent_protocol::extentid_t id;
    extent_protocol::attr a;
    
    printf("========== begin test remove ==========\n");
    for (i = 0; i < FILE_NUM; i++) {
        memset(&a, 0, sizeof(a));
        id = (extent_protocol::extentid_t)(i+2);
        if (ec->remove(id) != extent_protocol::OK) {
            iprint("error removing, return not OK\n");
            return 1;
        }
        ec->getattr(id, a);
        if (a.type != 0) {
            iprint("error removing, type is still positive\n");
            return 2;
        }
    }
    total_score += 20; 
    printf("========== pass test remove ==========\n");
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 1) {
        printf("Usage: ./lab1_tester\n");
        return 1;
    }
  
    ec = new extent_client();

    if (test_create_and_getattr() != 0)
        goto test_finish;
    if (test_put_and_get() != 0)
        goto test_finish;
    if (test_remove() != 0)
        goto test_finish;

test_finish:
    printf("---------------------------------\n");
    printf("Final score is : %d\n", total_score);
    return 0;
}
