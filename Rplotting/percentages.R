# This script can be used to compute the difference in percentage for one evaluation metric (either ISR or PUT) from one reference experiment to a new experiment

suppressPackageStartupMessages(library("argparse"))

parser <- ArgumentParser()

parser$add_argument("-r","--ref", help="absolute path of the file containing the reference experiment traced Data. A name without path will be only searched in the current wd", required=TRUE)
parser$add_argument("-n","--new", help="absolute path of the file containing the new experiment traced Data. A name without path will be only searched in the current wd", required=TRUE)

args <- parser$parse_args()

ref <- setNames(readRDS(file=args$ref),c("Time","Value"))
new <- setNames(readRDS(file=args$new),c("Time","Value"))

res = mapply('-',ref,new,SIMPLIFY=FALSE)[2]


indexes = which(res$Value != 0)


values = sapply(res, "[", indexes)


round(mean(values)*100)
