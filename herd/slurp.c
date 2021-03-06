#include "../bison.h"

/******************************************************************************
*
*   Remove a raw element from the start of a linked-list
*   is_ready(first, 0) must return 1!
*
*   struct packed_struct *first: first sentinel struct
*
*******************************************************************************/
void remove_raw_element(struct packed_struct *first) {
    struct packed_struct *remove = first->next;
    struct packed_struct *new_next = remove->next;

    first->next = new_next;
    free(remove->packed);
    free(remove);
}

/******************************************************************************
*
*   Move an element from one linked-list to another.
*
*   struct packed_struct *source: source linked list
*   struct packed-struct *dest: destination sentinel node
*
*******************************************************************************/
void move_element(struct packed_struct *source, struct packed_struct *dest) {
    struct packed_struct *next_to_last = dest->previous;
    struct packed_struct *element = source->next;
    struct packed_struct *new_next = NULL;

    //Remove from source
    new_next = source->next->next; //the next read
    source->next->previous = dest->previous; //Ensure that the read knows who came before it
    //Remove from source
    element->next = dest; //Next is the sentinel node
    element->state = 0; //read is set not ready
    dest->previous = element; //Update destination sentinel node

    //Update the source
    source->next = new_next;

    //Add to destination
    next_to_last->next = element; //Update previous read to point to the new one
    //Don't do anything if the previous node is a sentinel node
    if(next_to_last->previous != next_to_last) next_to_last->state = 1; //set previous read to ready
}

/******************************************************************************
*
*   Destroy a linked list of packed_structs with unmodified ->packed
*
*   struct packed_struct *first: linked list to destroy
*
*******************************************************************************/
void destroy_raw_list(struct packed_struct *first) {
    while(first->next->next != first->next) remove_raw_element(first);
    free(first->next);
    free(first);
}

/******************************************************************************
*
*   The MPI receiver thread on a bison_herd main node
*
*   void *a: NULL input
*
*   returns NULL
*
*******************************************************************************/
#ifndef DEBUG
void *herd_slurp(void *a) {
    time_t t0, t1;
    void *p = NULL;
    int nnodes = effective_nodes();
    int nfinished = 0;
    int source = 0;
    int size = 0;
    struct packed_struct *target_node = NULL;
    bam_hdr_t *tmp_header;
    MPI_Status status;
    if(MPI_Recv((void *) &size, 1, MPI_INT, 1, 1, MPI_COMM_WORLD, &status) != MPI_SUCCESS) {
        fprintf(stderr, "Received an error when trying to receive header size.\n");
        fflush(stderr);
        quit(3, -2);
    }
    p = malloc((size_t) size);
    assert(p);
    if(MPI_Recv(p, size, MPI_BYTE, 1, 2, MPI_COMM_WORLD, &status) != MPI_SUCCESS) {
        fprintf(stderr, "Received an error when trying to receive header.\n");
        fflush(stderr);
        quit(3, -2);
    }
    tmp_header= bam_hdr_init();
    unpack_header(tmp_header, p);
    free(p);
    global_header = tmp_header; //Now the writer thread is unblocked!
    
    t0 = time(NULL);
    if(!config.quiet) fprintf(stderr, "Started slurping @%s", ctime(&t0)); fflush(stderr);
    while(nfinished < nnodes) {
        MPI_Probe(MPI_ANY_SOURCE, 5, MPI_COMM_WORLD, &status);
        source = status.MPI_SOURCE;
        MPI_Get_count(&status, MPI_BYTE, &size);
        target_node = last_sentinel_node[source-1];

        if(size > 1) {
            p = malloc((size_t) size);
            assert(p);
            MPI_Recv(p, size, MPI_BYTE, source, 5, MPI_COMM_WORLD, &status);
            add_element(target_node, p);
        } else {
            p = malloc((size_t) size);
            assert(p);
            MPI_Recv(p, size, MPI_BYTE, source, 5, MPI_COMM_WORLD, &status);
            free(p);
            add_finished(target_node);
            nfinished++;
        }
    }
    t1 = time(NULL);
    if(!config.quiet) fprintf(stderr, "Finished slurping @%s\t(%f seconds elapsed)\n", ctime(&t1), difftime(t1, t0)); fflush(stderr);
    return NULL;
}
#else
void *herd_slurp(void *a) {
    time_t t0, t1;
    htsFile *fp1, *fp2, *fp3, *fp4, *fp5, *fp6, *fp7, *fp8;
    char *iname = malloc(sizeof(char) * (1+strlen(config.odir)+strlen(config.basename)+strlen("_X.bam")));
    bam1_t *read = bam_init1();
    bam_hdr_t *tmp;
    MPI_read *packed = calloc(1, sizeof(MPI_read));
    struct packed_struct *target_node = NULL;
    assert(iname);
    assert(packed);

    //Open the input files and get the header
    sprintf(iname, "%s%s_1.bam", config.odir, config.basename);
    fp1 = sam_open(iname, "r");
    global_header = sam_hdr_read(fp1);
    sprintf(iname, "%s%s_2.bam", config.odir, config.basename);
    fp2 = sam_open(iname, "r");
    tmp = sam_hdr_read(fp2);
    bam_hdr_destroy(tmp);
    sprintf(iname, "%s%s_3.bam", config.odir, config.basename);
    fp3 = sam_open(iname, "r");
    tmp = sam_hdr_read(fp3);
    bam_hdr_destroy(tmp);
    sprintf(iname, "%s%s_4.bam", config.odir, config.basename);
    fp4 = sam_open(iname, "r");
    tmp = sam_hdr_read(fp4);
    bam_hdr_destroy(tmp);
    sprintf(iname, "%s%s_5.bam", config.odir, config.basename);
    fp5 = sam_open(iname, "r");
    tmp = sam_hdr_read(fp5);
    bam_hdr_destroy(tmp);
    sprintf(iname, "%s%s_6.bam", config.odir, config.basename);
    fp6 = sam_open(iname, "r");
    tmp = sam_hdr_read(fp6);
    bam_hdr_destroy(tmp);
    sprintf(iname, "%s%s_7.bam", config.odir, config.basename);
    fp7 = sam_open(iname, "r");
    tmp = sam_hdr_read(fp7);
    bam_hdr_destroy(tmp);
    sprintf(iname, "%s%s_8.bam", config.odir, config.basename);
    fp8 = sam_open(iname, "r");
    tmp = sam_hdr_read(fp8);
    bam_hdr_destroy(tmp);
    free(iname);


    //Write a header
    global_header = modifyHeader(global_header, config.argc, config.argv);
    sam_hdr_write(OUTPUT_BAM, global_header);
    packed->size = 0;

    t0 = time(NULL);
    if(!config.quiet) fprintf(stderr, "Started slurping @%s", ctime(&t0)); fflush(stderr);

    while(sam_read1(fp1, global_header, read) > 1) {
        packed->packed = NULL;
        packed->size = 0;
        packed = pack_read(read, packed);
        target_node = last_sentinel_node[0];
        add_element(target_node, packed->packed);

        //Node2
        sam_read1(fp2, global_header, read);
        packed->packed = NULL;
        packed->size = 0;
        packed = pack_read(read, packed);
        target_node = last_sentinel_node[1];
        add_element(target_node, packed->packed);

        //Node3
        sam_read1(fp3, global_header, read);
        packed->packed = NULL;
        packed->size = 0;
        packed = pack_read(read, packed);
        target_node = last_sentinel_node[2];
        add_element(target_node, packed->packed);

        //Node4
        sam_read1(fp4, global_header, read);
        packed->packed = NULL;
        packed->size = 0;
        packed = pack_read(read, packed);
        target_node = last_sentinel_node[3];
        add_element(target_node, packed->packed);

        //Node5
        sam_read1(fp5, global_header, read);
        packed->packed = NULL;
        packed->size = 0;
        packed = pack_read(read, packed);
        target_node = last_sentinel_node[4];
        add_element(target_node, packed->packed);

        //Node6
        sam_read1(fp6, global_header, read);
        packed->packed = NULL;
        packed->size = 0;
        packed = pack_read(read, packed);
        target_node = last_sentinel_node[5];
        add_element(target_node, packed->packed);

        //Node7
        sam_read1(fp7, global_header, read);
        packed->packed = NULL;
        packed->size = 0;
        packed = pack_read(read, packed);
        target_node = last_sentinel_node[6];
        add_element(target_node, packed->packed);

        //Node8
        sam_read1(fp8, global_header, read);
        packed->packed = NULL;
        packed->size = 0;
        packed = pack_read(read, packed);
        target_node = last_sentinel_node[7];
        add_element(target_node, packed->packed);
    }
    free(packed);
    bam_destroy1(read);
    sam_close(fp1);
    sam_close(fp2);
    sam_close(fp3);
    sam_close(fp4);
    sam_close(fp5);
    sam_close(fp6);
    sam_close(fp7);
    sam_close(fp8);

    target_node = last_sentinel_node[0];
    add_finished(target_node);
    target_node = last_sentinel_node[1];
    add_finished(target_node);
    target_node = last_sentinel_node[2];
    add_finished(target_node);
    target_node = last_sentinel_node[3];
    add_finished(target_node);
    target_node = last_sentinel_node[4];
    add_finished(target_node);
    target_node = last_sentinel_node[5];
    add_finished(target_node);
    target_node = last_sentinel_node[6];
    add_finished(target_node);
    target_node = last_sentinel_node[7];
    add_finished(target_node);

    t1 = time(NULL);
    if(!config.quiet) fprintf(stderr, "Finished slurping @%s\t(%f seconds elapsed)\n", ctime(&t1), difftime(t1, t0)); fflush(stderr);
    return NULL;
}
#endif
