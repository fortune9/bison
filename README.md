#Bison: bisulfite alignment on nodes of a cluster.

##Prerequisites

This program depends upon the following:

1. A functional MPI implementation, such as mpich

2. The SAMtools library or similar. SAMtools is available here: http://samtools.sourceforge.net/
   Note, the development version of samtools available on github that uses
   htslib is currently not compatible.

3. Bowtie2, available here: http://bowtie-bio.sourceforge.net/bowtie2/index.shtml
   The bowtie2 executable MUST be in your PATH.

4. zcat, gzip, and bzcat must also be in your PATH, though this will almost
   always be the case.

5. To use bison_mbias2pdf (or the -pdf option of bison_mbias), R must be
   installed and in your PATH. Additionally, the ggplot2 library must be
   installed.

N.B., the actual SAMtools library and header files are required for the
   compilation step and can then be removed. The actual samtools executable
   isn't required.

##General setup should go as follows:

0. Download and extract the source code for samtools. Change into the directory
   containing said code and type "make".

1. Download the source distribution.

2. Unpack, for example: tar zxf bison-0.1.0.tgz

3. Possibly edit the Makefile, to include MPI and SAMtools library and header
   locations. If these are installed in standard locations, the defaults
   should suffice. For samtools see example in the Makefile. The default
   Makefile is suitable for mpich2. If you're using openmpi you'll need to
   comment out the first MPI line and uncomment the second MPI line.

4. type "make"

   * If you would like to use `bison_herd`, type "make herd".

   * If you would like the auxiliary tools installed, type "make auxiliary".

5. type "make install"

The install path can be changed easily in the Makefile.

##Detailed installation instructions

1. Download samtools (at least version 0.1.19!).

2. Extract the compressed bzipped tar-ball:
tar jxf samtools-0.1.19.tar.bz2

3. Change to that directory and type:
make

4. Similarly download and extract the source code for bison

5. Change the installation target. For example, if you would like bison to be
   installed under "bin" in your home directory, then the PREFIX line should be:
PREFIX = ~/bin

6. The default compiler is mpicc, but this can be changed by altering the line
   beginning with "CC".

7. If you extracted and built samtools in your home directory, then you will
   likely need to change the `INCLUDE_DIRS` and `LIB_DIRS` to something like:

    INCLUDE_DIRS = -I/home/username/samtools-0.1.19
    LIB_DIRS = -L/home/username/samtools-0.1.19

   If you already have the headers and libbam.a file elsewhere, then change
   these lines appropriately.

   Likewise, add the location of your MPI headers and libraries, if they're not
   in the normal search path.

8. You can disable throttling in `bison_herd` by adding "-DNOTHROTTLE" in the
   "OPTS" line, though read the "Throttling" section , below. Similarly, both
   bison and `bison_herd` can be compiled in a special debug mode by adding
   "-DDEBUG" to the "OPTS" line. See the "Debug mode" section, below.

9. Continue with step #4 in the preceding section.

##Usage

One can index all fasta files (files with extension .fa or .fasta) in a
directory as follows:

    bison_index [OPTIONS] directory/

Options that are not specific to bison are simply passed to bowtie2, which must
be in your PATH. The output is placed under `directory/bisulfite_genome`.

Alignment can be performed as follows (`bison_herd` is the same):

    mpiexec bison [OPTIONS] -g directory/ {-1 fastq_1.gz -2 fastq_2.gz | -U fastq.fq}

"directory" is identical to that used for indexing. For further details type
"bison -h". For non-directional libraries, "mpiexec -N 5" should be used,
otherwise "mpiexec -N 3". Resource managers, such as slurm, should work in
an equivalent manner. All options not explicitly mentioned by typing
"bison -h" are passed to bowtie2. Consequently, using the --very-sensitive or
--dovetail options will work as expected. Bison already passes the following
flags to bowtie2:

    -q --reorder --no-mixed --no-discordant

`bison_herd` is equivalent, except that you can specify more nodes. You may also
input multiple files (comma-separated, no spaces) to align, in which case
alignments will be printed to multiples files. Furthermore, you may use
wild-cards in your file list. For example:

    mpiexec -N 17 bison_herd -o Alignments -g directory/ -1 exp1/sample*_1.fq.gz,/some/other/path/foo*_1.fq.gz -2 exp1/sample*_2.fq.gz,/some/other/path/foo*_2.fq.gz

Make sure to not have multiple input files with the same name
(e.g., `sample*/read1.fastq`), as they will all be written to the same file
(overwriting any subsequent alignments)!

In non-targeted sequencing experiments, it is usually wise to mark likely PCR
duplicates. These are then ignored by the methylation extractor so as to not
increase the error rate of any particular position. `bison_markduplicates` and
able to process a BAM file generated by `bison`/`bison_herd` and produce an
identical BAM file with the 0x400 bit set in the FLAG field for reads that are
likely duplicates. This step is not required and should be avoided if you are
performing RRBS or other targeted sequencing, as the false-positive rate of will
then be too high.

There is also a methylation extractor that produces a bedGraph file, called 
`bison_methylation_extractor`. Note, coordinate-sorted BAM files should not 
be used! The methylation extractor can be told to ignore certain parts of each
read. This is particularly useful in cases where there is methylation bias
across the length of reads (i.e., if one plots the average methylation
percentage summed per position over all reads, the value goes up/down toward the
5' or 3' end). It is recommended to always run `bison_mbias` (with the -pdf option
if you have R and ggplot2 installed) to generate the required information for
constructing an M-bias plot. The `bison_mbias2pdf` script can convert this to a
PDF file (or a series of PNG files) and will also suggest what, if any, regions
should be ignored. These regions are strand and read number (in the case of
paired-end reads) dependent. While the suggested regions are often good, the
should not be blindly accepted (just look at the graph and use your best
judgement).

See the "Auxiliary files" section, below, for additional files.

##Auxiliary files

The following programs and scripts will be available if you type "make auxiliary":

###bedGraph2BSseq.py
This python script can accept a filename prefix and the names of at least 2
bedGraph files and output 3 files for input into BSseq. A single chromosome can
be processed at a time, if desired, by using the -chr option. The output files
will be named $prefix.M, $prefix.Cov, and $prefix.gr. $prefix.M is a matrix with
a header line that lists the number of reads supporting methylation at each site
in the bedGraph files. If there is no coverage in a given sample, the value is
set to 0. $prefix.Cov is the analogous file listing coverage in each sample
(again, 0 denotes no coverage). $prefix.gr lists the coordinates for each line
in the .Cov and .M files. Loading these files into R would be performed as
follows (in this example "Chr17" was the prefix):

```R
M <- as.matrix(read.delim("Chr17.M", header=T))
Cov <- as.matrix(read.delim("Chr17.Cov", header=T))
bed <- read.delim("Chr17.bed", header=F)
#Remember that BED and bedGraph files are 0-based!
gr <- GRanges(seqnames=Rle(bed$V1),ranges=IRanges(start=bed$V2+1, end=bed$V3), strand=Rle("*", nrow(bed)))
groups <- data.frame(row.names=colnames(M),
    var1 <- c(1,1,1,1,2,2,2,2)) #A very simple experiment with 2 groups of 4 samples
BS1 <- BSseq(M=M, Cov=Cov, gr=gr, pData=groups, sampleNames=colnames(M)) #You'll want to set some of the additional options!
```


###`bedGraph2methylKit`
As above, but each bedGraph file is converted to a .methylKit file. The
bedGraphs should be of CpGs and not have had the strands merged (i.e., don't run
the merge_CpGs command below).

###`bedGraph2MOABS`
Like `bedGraph2methylKit`, but each bedGraph file is converted to a .moabs file.
The bedGraph files should ideally contain single-C metrics rather than having
been merged to form CpG metrics, though both are supported. The resulting .moabs
files can then be used by `mcomp` in the MOABS package.

###`bedGraph2MethylSeekR`
As above, but each bedGraph file is converted into a .MethylSeekR file. The
bedGraphs MUST be merged before-hand with bison_merge_CpGs to create per-CpG
metrics, as this is what MethylSeekR is expecting. Input is performed with the
`readMethylome()` function. Chromosome lengths can be computed with:
samtools faidx genome.fa
where genome.fa is a multifasta file containing all of the chromosomes. The
resulting .fai file is simply a text file and can be loaded into R with:

```R
fai <- read.delim("genome.fa.fai", header=F)
chromosome_lengths <- fai$V2
names(chromosome_lengths) <- fai$V1
d <- readMethylome("file.MethylSeekR", chromosome_lengths)
```

###`make_reduced_genome`
Create a reduced representation genome appropriate for reads of a given size
($size, default is 36bp). MspI and TaqI libraries are supported. Nucleotides
greater than $size+10% are converted to N.

###`merge_bedGraphs.py`
This will merge bedGraphs from technical replicates of a single sample into a
single bedGraph file, summing the methylation metrics as it goes. The output,
like the input is coordinate sorted.

###`bison_merge_CpGs`
Methylation is usually symmetric at CpG sites. While the output bedGraph files
have a single-C resolution, this will convert that to single-CpG resolution by
summing Cs in the same CpG from opposite strands. This saves space and will
often speed up downstream statistics.

##Importing into other analysis packages
While there are helper scripts, mentioned above, for a number of packages, other
packages either do not require a helper script or can use one of the
aforementioned scripts. Import instructions for such packages are mentioned
below.

###BiSeq
BiSeq requires input in an identical format as BSseq. Consequently, just use the
bedGraph2BSseq.py helper script. The following example commands should then
suffice to load everything into R:

```R
exptData <- SimpleList(Sequencer="Some sequencer", Year="2014") #This is just descriptive information
M <- as.matrix(read.delim("chr17.M", header=T))
Cov <- as.matrix(read.delim("chr17.Cov", header=T))
bed <- read.delim("chr17.bed", header=F)
gr <- GRanges(seqnames=Rle(bed$V1),ranges=IRanges(start=bed$V2+1, end=bed$V3), strand=Rle("*", nrow(bed)))
groups <- DataFrame(row.names=colnames(M),
    group = c(1,1,1,1,2,2,2,2)) #A very simple experiment with 2 groups of 4 samples
d <- BSraw(exptData=exptData, rowData=gr, colData=groups, totalReads=Cov, methReads=M)
```

###BEAT
The BEAT Bioconductor package conveniently expects per-sample position and
methylation information in a format already present in bedGraph files. However,
this information is in a slightly different format than bedGraph, so the
following awk script can be used. Note that BEAT expects files named as
sample_name.positions.csv.

    awk '{if(NR>1){printf("%s,%i,%i,%i\n",$1,$2+1,$5,$6)}else{printf("chr,pos,meth,unmeth\n")}}' sample.bedGraph > sample.positions.csv


##Advanced bison_herd usage

`bison_herd` has the ability to use a semi-arbitrary number of nodes. In practice,
if bison is given N nodes, it will effectively use `2*((N-1)/2)+1` or
`4*((N-1)/4)+1` nodes, for directional and non-directional libraries,
respectively. As an example, if you allot 20 nodes for a directional library,
`bison_herd` will only use 19 of them (17 for non-directional reads). The excess
nodes will exit properly and, unless you specify --quiet, produce an error
message.

The options -mp, -queue-size, and -@ are `bison_herd`-specific and deserve further
description.

-mp sets the number of threads that the master node will use to process
alignments produced by the worker nodes. Worker nodes are grouped into twos or
fours, where each group has the a number of nodes equal to the number of
possible bisulfite converted strands. As the number of allocated nodes
increases, a point is eventually reached where a single thread on the master
node is unable to keep up with the workers. In my experience, for directional
libraries, one thread can handle approximately 130 bowtie2 threads (i.e., if
using -p 11, -mp should be increased once ~12 worker nodes are allocated, since
that would equate to 132 threads in use by bowtie2). One should keep in mind
that there are already at least 3 other threads concurrently running on the
master node (sending and storing fastq reads, receiving alignments, and writing
alignments). Consequently, there is a practical limit to the number of nodes is
determined by how many cores are available on each node.

-queue-size determines the maximum difference between reads sent for alignment
and reads processed. This option is unavailable if `bison_herd` was compiled with
-DNOTHROTTLE. By default, the thread that sends reads for alignment will pause
if it has sent more than ~1 million reads than have been processed. The purpose
of this is to prevent overwhelming of the MPI unexpected message buffer, since
the thread on the master node that sends reads can generally process reads
faster than all of the worker nodes combined can align them. Setting this value
too high may result in `bison_herd` crashing with otherwise cryptic messages
involving `MPI_Send`. In such cases, decreasing the value used by -queue-size
should resolve the problem. On the other hand, setting this value too low can
result in a deadlocks, due to buffering at various levels. The default value
hasn't resulted in deadlocking or crashes on our cluster, but yours may be
different! This difference is checked every 100000 reads, which can changed by
editting the `THROTTLE_CHECK_INTERVAL` value in bison.h prior to compilation.

-@ specifies the number of compression threads used for writing the output BAM
file. In practice, a single compression thread can write ~80 million paired-end
reads per hour (depending on CPU speed). I routinely use -@ 4 when using more
than ~9 nodes as this allows writing to occur as quickly as reads are processed.
To determine if the number of compression threads should be increased, not the
time difference (especially early on) between when each master processor thread
has processed 100000 reads and when those reads have been written to a file.
Even when --reorder is used, if there is >1 second between these, then you may
benefit from increasing the number of compression threads. For those curious,
this option is identical to that used in samtools.

##Throttling

`bison_herd` generally uses blocking, but not synchronous sends. What this means
in practice is that many reads will be queued by the master node for sending to
the worker nodes. Likewise, many alignments can be queued by the worker nodes
for sending back to the master node. The queue that many MPI implementations use
for this is relatively small and immutable. While a full queue should cause
`MPI_Send` to block until there is sufficient space, occasionally a constellation
of events can occur that cause this queue to overflow and the master node to
then crash. This can be alleviated by limiting the possible number of reads that
could ever possibly be in the queue at any single time. As the queue is not
directly pollable, the difference between the number of reads sent and written
is used as a surrogate. The maximum number of reads in the wild is then either
2x or 4x this difference (since a read is queued per worker node). In reality,
the queue should be emptier than this as there are normally reads buffered on
the worker nodes (being fed to bowtie2, being aligned or being sent) and
elsewhere on the master node (being received, waiting to be processed, being
processed, waiting to be written, or being written).

Throttling is not always required, particularly as an increasing number of nodes
are used. Throttling can be disabled altogether by compiling with -DNOTHROTTLE,
which will remove all related components.

##Debug mode

For debugging, a special debug mode is available for both bison and `bison_herd`
by compiling with -DDEBUG. Instead of running of needing multiple nodes, both
programs will then run as if they were just a single node. Compiling with this
option adds the -taskid option to both programs. The taskid is equivalent to the
node number in the bison (or `bison_herd`) hierarchy. Node 0 is the master node
and performs the final file writing. For bison, nodes 1-4 are equivalent to the
worker nodes that align reads to the original top, original bottom,
complementary to original top and complementary to original bottom strands,
respectively. For directional libraries, only the first 2 are used. These will
write alignments to a file for final processing when run as taskid 0. This is
useful when odd alignments are being output and the source of the error needs to
be tracked down. The mode for `bison_herd` is similar, except there are always 8
theoretical worker nodes (i.e., taskid 1-8 need to be run prior to taskid 0).
This allows testing multiple master processor threads with both directional and
non-directional reads.

In general, this mode should not be used unless you are running into extremely
odd bugs.

##Compatibility with Bismark

Bison is generally similar to bismark, however the indexes are incompatible,
due to bismark renaming contigs. Also, the two will not produce identical 
output, due to algorithmic differences. Running `bison_methylation_extractor`
on the output of bismark will also produce different results, again due to
algorithmic differences. In addition, bison always outputs BAM files directly.

##Other details

Bison needn't be run on multiple computers. You can also use a single
computer for all compute nodes (e.g. mpiexec -n 5 bison ...). The same holds
true for `bison_herd`. Both bison and `bison_herd` seem to be faster than bismark,
even when limited to the same resources.

##Changes

###0.3.2
  *  Added bedGraph2MOABS to convert bedGraph files for use by MOABS. See usage
     above.

  *  Added support for HTS-lib (you'll need to add -DHTSLIB to the OPTS line of
     the Makefile).

  *  Fixed a small bug wherein --reorder wasn't being invoked when multiple
     output BAM files were to be used.

###0.3.1
  *  The various bedGraph files didn't previously have a "track" line. The UCSC
     Genome Browser requires this, so bedGraph files produced will now contain 
     it. It should be noted that this is the very minimal line required. Bison
     does not provide facilities for making these changes, users need to edit
     things manually or use external programs for this. It should also be noted
     that any changes to the "track" or other header lines should be made after
     all processing with Bison is complete.

  *  Add conversion scripts for import into MethylSeekR, BiSeq, and BEAT.

  *  Revamped how `bison_markduplicates` works. The 3' coordinates are now
     ignored, soft-clipped bases on the 5' end are now incorporated in
     determining the 5' coordinate and methylation calls are also used in
     determining if reads/pairs are duplicates. This should be a much more
     robust (though more resource intensive) method than that previously used.
     Whereas the previous version kept unmarked the read/pair with the highest
     MAPQ, this one will do that for the read/pair with the highest summed phred
     score (a la picard).

###0.3.0
  *  Note: The indices produced by previous versions are not guaranteed to be
     compatible unless you used a multi-fasta file. There was a serious
     implementation problem with how `bison_index` worked when given multiple
     files as input and how multiple files were read into memory in previous
     versions. If you used a multi-fasta file, then everything will continue
     to work correctly. However, if you used multiple fasta files in a list
     then I strongly encourage you to delete the previous indices (just remove
     the bisulfite_genome directory) and reindex. The technical reasons for this
     issue are that when the bison tools previously read multiple fasta files
     into memory, they would do so in whatever order they appeared in the
     directory structure, which can change over time and isn't guaranteed to
     match the order of files someone specified during indexing. While the
     alignments wouldn't be affected by this, the methylation calls could have
     been seriously compromised. In this version, bison_index will only accept a
     directory, not a list of files, and it will always alphasort() the list of
     files in that directory prior to processing. This should eliminate this
     problem. My apologies to anyone affected by this.

  *  Added --genome-size option to a number of the tools. Many of the bison 
     programs need to read the genome into memory. By default, 3 gigabases worth
     of memory are allocated for that and the size increased as needed. For
     smaller genomes, this wasted space. For larger genomes, the constant
     reallocation of space could seriously slow things down. Consequently, this
     option was added to any tool that reads the genome into memory. It's
     convenient to overestimate this slightly, so if your genome is 3.8
     gigabases, then just use 4000000000 as the genome size.

  *  `bison_merge_CpGs` can now take multiple input files at once.

  *  A number of small bug fixes, such as when "genome_dir" doesn't end in a /.

###0.2.4
  *  Fixed an off-by-one error in bison_mbias. Also, at some point 1-methylation
     percentage started getting calculated. That's been fixed.

  *  Added bison_markduplicates, which, as the name implies, marks apparent PCR
     duplicates. The methylation extractor and m-bias calculator have also been
     updated to ignore marked duplicates.

  *  Fixed a bug in the CpG coverage program, which wasn't properly handling
     single-C bedGraph files before (if they were merged, then they were being
     handled correctly).

###0.2.3
  *  Fix how hard and soft-clipped bases are dealt with (previously, soft-
     clipped bases resulted in an error and hard-clipped bases in incorrect
     position assignments!).

  *  Multiple bug fixes related to local alignment, which previously didn't
     work correctly. These issues seem to generally now be resolved. May thanks
     to user mvijayen on seqanswers for providing a perfect usage example for
     testing (see thread http://seqanswers.com/forums/showthread.php?t=39914).

  *  The maximum length of a single contig is now (2^64)-1 (instead of the
     previous 2^64). I don't think bowtie2 would even support something that
     long, but if it did then bison wouldn't (internally, a position of 2^64
     means a base is inserted, soft, or hard-clipped).

  *  A previously missing "*" caused Bison to use the entirety of the
     description line in the fasta file as the chromosome name. This caused
     errors since bowtie2 only uses every before the first space (the proper
     method). Bison now does the same.

  *  A note about creating methylation-bias metrics with locally aligned reads
     is in order. If a read is soft-clipped, that portion is still included in
     the M-bias metrics. Likewise, if you pass -OT X,X,X,X or similar
     parameters to the methylation extractor, the soft-clipped area is also
     included in there.

  *  Another note regarding local alignments is that the XX auxiliary tag
     (effectively the more verbose version of the MD tag) contains soft-clipped
     sequences. I could probably have these removed if someone would like.

###0.2.2
  *  Properly fixed some wording on the textual output (i.e., removed the word
     "unique").

  *  Lowered the default MAPQ and Phred thresholds used by the methylation
     extractor to 10 each. That the MAPQ threshold was originally
     20 was an error on my part.

###0.2.1
  *  Added support for file globbing in bison_herd. You may now input multiple
     files using a combination of wild-cards (*, ?, etc.) and commas. Remember
     to put these in quotes (e.g., "foo/*1.fq.gz","bar/*1.fq.gz") so the shell
     doesn't perform the expansion!). As before, specifying multiple inputs with
     the same file name (e.g., sample1/reads.fq,sample2/reads.fq) will cause the
     output from the first reads.fq alignment to be over-written by the second.

  *  Fixed the text output, since "unique alignments" isn't really correct,
     given that alignments with scores of 0 or 1 can be output but aren't
     unique.

  *  Added information in the Makefile and above about compiling with openmpi.

  *  Fixed a bug in bison_herd wherein the -upto option wasn't being handled
     properly. -upto now accepts an unsigned long in bison_herd.

  *  Fixed a bug in bison_herd when paired-end reads were used. This was due to
     how bowtie2 reads from FIFOs. Changing how things were written to the FIFOs
     on the worker nodes resolved the problem.

  *  The bison_mbias program has been heavily revamped. It still outputs the
     number of methylated or unmethylated CpG calls per position, but now keeps
     the metrics for each strand (and read, when paired-end reads are used)
     separate. If R and the ggplot2 library are installed, the program can also
     run the bison_mbias2pdf program (see below).

  *  Created an bison_mbias2pdf Rscript that will read in the output of
     bison_mbias and plot the results, indicating the region of each read that
     should be included in methylation extraction. This script also print these
     suggestions in the format used by bison_methylation_extractor, for
     convenience.

  *  The methylation extractor can now be told to only include certain regions
     of each read in the output methylation metrics. This is needed when there
     is apparent bias in the methylation at one or both ends of a read. 

  *  Previously, the recalculated MAPQ was incorrect when only 1 read in a pair
     had a valid secondary alignment. This has been fixed.

  *  Fixed another MAPQ recalculation bug, affecting reads with MAPQ 2 that
     have MAPQ=6.

  *  Fixed a bug in writing unmapped reads.

  *  Fixed a bug in bison_herd that allowed early termination without warning.

###0.2.0
  *  Added a note to the methylation summary statistics output at the end of a
     run that the numbers will include double counting of any site covered by
     both mates in a pair. These metrics are only meant for general information
     and not further analysis, so I don't consider that a bug (it's actually a
     design decision for the sake of performance).

  *  --ignore-quals is no longer passed to bowtie2 by default. Specifying this
     will marginally decrease both correct and incorrect alignments. It will
     also generally decrease the alignment rate.

  *  Fixed --unmapped, which are now written to the directory specified by -o

  *  --maxins was already 500 by default, so it is no longer set by default.

  *  Added bison_herd, see above for usage

  *  The methylation extractor now has a -phred option, to exclude methylation
     calls from low confidence base-calls. The default threshold is 20.

  *  Added a script to convert bedGraph files to a format suitable for BSseq.

  *  Fixed a bug in bison_merge_CpGs

  *  Both bison and bison_herd now check to ensure that the MPI implementation
     actually supports the level of thread support requested (previously, this
     was just assumed).

###0.1.1
  *  Fixed a number of minor bugs.

  *  Added support for uncompressed fastq files, as well as bzipped files
     (previously, only gzipped fastq files worked properly).

  *  --score-min is now parsed by bison prior to being sent to bowtie2,
     read MAPQ scores are recalculated accordingly by the same algorithm
     used by bowtie2 (N.B., this only bears a vague correspondence to
     -10*log10(probability the mapping position is wrong)!).

  *  Added a bison_mbias function, to process the aligned BAM file and
     create a text file containing the percentage of methylated C's as a
     function of read position. For the utility of this, see: Hansen KD,
     Langmead B and Irizarry RA, BSmooth: from whole genome bisulfite
     sequencing to differentially methylated regions. Genome Biol 2012;
     13(10):R83.

  *  The methylation extractor now accepts the -q options, which sets the
     MAPQ threshold for a read to be included in the methylation results.
     The default is a minimum MAPQ of 20, which seems to be a reasonable
     threshold from a few simulations.

  *  In DEBUG mode, the output BAM files used to have fixed names. This was
     a problem in cases where debugging was being performed on multiple
     input files. Now, the OT/OB/CTOT/CTOB.bam filename is prepended with
     an appropriate prefix (extracted from the input file name). In
     addition, the output directory is now respected in DEBUG mode.

  *  Included an "auxiliary" directory, that includes functions for making
     an RRBS genome and other possibly useful functions.


###0.1.0
  Initial release
