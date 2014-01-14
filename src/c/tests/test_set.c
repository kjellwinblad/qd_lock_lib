#include "tests/test_framework.h"

#include "data_structures/sorted_list_set.h"
#include "data_structures/chained_hash_set.h"
#include "data_structures/sets.h"
#include "misc/random.h"


void * my_extract_key(void * v, int keyPos){
    (void)keyPos;
    return v;
}

unsigned int my_hash_key(void * keyPtr){
    int key = *((int *)keyPtr);
    return ((unsigned int)key);
}

bool my_are_equal(void * k1, void * k2){
    int  k1i = *((int *)k1);
    int k2i = *((int *)k2);
    return k1i == k2i;
}

char * my_to_string(void * value){
    int  valuei = *((int *)value);
    char * str = malloc(15);
    sprintf(str, "%d", valuei);
    return str;
}

SET_TYPE * create_set(){
    return (SET_TYPE *)SET_create(SET_TYPE_NAME,
                      0,
                      my_extract_key,
                      my_hash_key,
                      my_are_equal,
                      my_to_string);
}


int test_create_and_delete(){
    SET_TYPE * set = create_set();
    SET_free(set);
    return 1;
}

int test_insert(){
    SET_TYPE * set = create_set();
    int x = 10;
    SET_insert(set, &x, sizeof(int));
    assert(*((int *)SET_lookup(set, &x)) == 10);
    SET_free(set);
    return 1;
}

int test_insert_new(){
    SET_TYPE * set = create_set();
    int x = 10;
    assert(SET_insert_new(set, &x, sizeof(int)));
    assert(*((int*)SET_lookup(set, &x)) == 10);
    assert(!SET_insert_new(set, &x, sizeof(int)));
    SET_free(set);
    return 1;
}

int test_insert_write_over(){
    SET_TYPE * set = create_set();
    int x = 10;
    SET_insert(set, &x, sizeof(int));
    x = 5;
    SET_insert(set, &x, sizeof(int));
    x = 15;
    SET_insert(set, &x, sizeof(int));
    x = 10;
    SET_insert(set, &x, sizeof(int));
    assert(*((int*)SET_lookup(set, &x)) == 10);
    SET_free(set);
    return 1;
}

int test_lookup(){
    SET_TYPE * set = create_set();
    int x = 42;
    SET_insert(set, &x, sizeof(int));
    assert(*((int*)SET_lookup(set, &x)) == 42);
    SET_free(set);
    return 1;
}

int test_lookup_not_exsisting(){
    SET_TYPE * set = create_set();
    int x = 42;
    SET_insert(set, &x, sizeof(int));
    x = 43;
    assert(SET_lookup(set, &x) == NULL);
    SET_free(set);
    return 1;
}

int test_remove(){
    SET_TYPE * set = create_set();
    int x = 42;
    SET_insert(set, &x, sizeof(int));
    assert(*((int*)SET_lookup(set, &x)) == 42);
    SET_delete(set, &x, sizeof(int));
    assert(SET_lookup(set, &x) == NULL);
    SET_free(set);
    return 1;
}


int test_insert_lookup_delete_lookup_many(){
    int nrOfOpsOfEachType = 1024;
    SET_TYPE * set = create_set();
    int elements[nrOfOpsOfEachType];
    unsigned int seed = 13;
    //Create elements
    for(int i = 0; i < nrOfOpsOfEachType; i++){
        elements[i] = (int)(10000*random_double(&seed));
    }
    //Inserts
    for(int i = 0; i < nrOfOpsOfEachType; i++){
        SET_insert(set, &elements[i], sizeof(int));
    }
    //Inserts again!
    for(int i = 0; i < nrOfOpsOfEachType; i++){
        assert( ! SET_insert_new(set, &elements[i], sizeof(int)));
    }
    //Lookups
    for(int i = 0; i < nrOfOpsOfEachType; i++){
        assert(*((int*)SET_lookup(set, &elements[i])) == elements[i]);
    }
    //Deletes
    for(int i = 0; i < nrOfOpsOfEachType; i++){
        SET_delete(set, &elements[i], sizeof(int));
    }
    //Lookup_again
    for(int i = 0; i < nrOfOpsOfEachType; i++){
        assert(SET_lookup(set, &elements[i]) == NULL);
    }
    SET_free(set);
    return 1;
}


int main(int argc, char **argv){
    (void)argc;
    (void)argv;
    T(test_create_and_delete(), "test_create_and_delete()");
    T(test_insert(), "test_insert()");
    T(test_insert_write_over(), "test_insert_write_over()");
    T(test_lookup(), "test_lookup()");
    T(test_lookup_not_exsisting(), "test_lookup_not_exsisting()");
    T(test_remove(), "test_remove()");
    T(test_insert_lookup_delete_lookup_many(), "test_insert_lookup_delete_lookup_many()");
    return 0;
}

