#!/usr/bin/env Rscript

# Copyright (C) 2016, the University of Luxembourg
# Salvatore Signorello <salvatore.signorello@uni.lu>
# 
# This is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this.  If not, see <http://www.gnu.org/licenses/>.

# Plotting on the same graph ISR results from different experiments
suppressPackageStartupMessages(library("argparse"))

#commandArgs(trailingOnly = TRUE)
# create the parser object
parser <- ArgumentParser()

parser$add_argument("-d","--dir", help="absolute path of the directory containing the traced Data", required=TRUE)
parser$add_argument("-s","--script", help="Kind of script to be sourced: isr or pur", default="isr")
parser$add_argument("-n","--node", help="Kind of node: monitor or all. This option is valid only if the script option is pur")

args <- parser$parse_args()
sources_dir = args$dir
node_type = args$node

# Simple arrays containing labels to be used that varies according to the script arg
script_labels <- vector()

if (args$script == "isr") {
  script_labels <- c(script_labels, "./isr.R","isr","ISR","Interest Satisfaction Ratio","Satisfied legitimate Interests (%)","ISR")
} else if (args$script == "pur") {
  script_labels <- c(script_labels, "./pitUsage.R","pit_avg","PUR","Pit Utilization Ratio", "PIT usage (%)","PUR")
  if ( is.null(args$node) ){
      cat ("No node option specified for plotting the PitUlizationRatio, so only monitor nodes are considered\n") 
      node_type = "monitor"
  }
}


names(script_labels) <- c("Name","Res_df", "Res_column","title_main","title_ylab","line_column")


files <- list.files(path=sources_dir, pattern="*.txt", full.names=T, recursive=FALSE)

nfiles <- length(files)
cat("The source dir contains ", nfiles, " files")

# the tag is extracted from the file name which looks like follows
# topo=AS-3967_delays.ns3.txt_mar=2_ftbm=1_detection=3_numServers=3_payloadSize=1100_numClients=42@100_numAttackers=14@1000_numMonitors=8_tau=0.3_observationPeriod=10s_gamma=0.5_cacheSize=0_pitSize=5000_pitLifetime=2s_run=1_seed=9-l3trace.txt
filename <- basename(files[1])
name_components <- unlist(strsplit(filename,"_"))
# I define some labels to mark the defense mechanism as on or off which are more readable of the one included in the filename
def_label <- c("WD","ND")
names(def_label) <- c("mar=2","mar=0")
exp_label <- c("Wiki","Plain")

# the output name looks like: "def_label-attackerFrequency ipps", e.g., "WD-500.ipps"
tag <- paste(def_label[name_components[3]], paste(unlist(strsplit(name_components[9],"@"))[2],"ipps",sep="."), sep="-")

# the output of lapply should be a list with each term containing a dataset with the ISR of a different simulation
isrs <- lapply(files, function(x) {
  # call isr.R and get the data
  cat("Processing file: ", x, "\n")
  ee <- new.env()
  if (args$script == "isr"){
    commandArgs <- function() c("-f", x) 
  } else if (args$script == "pur"){
    commandArgs <- function() c("-f", x, "-n",node_type,"-e") 
  }
  #source("./isr.R", LOCAL=TRUE)
  sys.source(script_labels["Name"], ee)
  tmp <- script_labels["Res_df"]
  ee[[tmp]]
})
#TODO add a check to the elements got by the previous lapply and exit if you have lists with empty elements
# merge the list elements in a single data frame
merged_isrs <- as.data.frame(do.call("rbind",isrs))

# check the expected size - quick and dirty TBR
if (args$script == "isr"){
  if( nrow(merged_isrs) != (nfiles * 53) ){
    stop("After merge, the data-frame has not the expected number of rows")
  }
}

# compute the avg for time interval
final_column_names <- vector()
if (args$script == "isr"){
  final_column_names <- c(final_column_names, "Time","ISR")
} else if (args$script == "pur"){
  final_column_names <- c(final_column_names, "Time","PUR")
}
# the next is not going to work for the Pit script until we don't change the column name on the original script - I don't know what that meant and if it's still valid
# Apologies for the "ifelse(args$script == "pur",node_type,"")" repeated 3 times, I would probably have done better by defining a C-style ?: ternary myself, but I need a quick fix and cannot spend more time right now
saveRDS(merged_isrs, file=paste(args$dir, paste(tag,"merged",script_labels["Res_column"],ifelse(args$script == "pur",node_type,"all") ,"rds", sep="."), sep="/"))
final_isr <- setNames(aggregate(merged_isrs[script_labels["Res_column"]],list(merged_isrs$Time),FUN=mean),final_column_names)

# save the final dataframe, this will be loaded by a script plotting different experiments on the same plot, like for example, different attacker frequency or different defense mechanism.
saveRDS(final_isr, file=paste(args$dir, paste(tag,"final",script_labels["Res_column"], ifelse(args$script == "pur",node_type,"all"), "rds", sep="."), sep="/"))

# plot everything together (this code is pretty similar to the one used in the isr.R, I should consolidate it and make a single function somewhere)
out_file <- paste(paste(args$dir, paste(tag,"merged",script_labels["Res_column"], ifelse(args$script == "pur",node_type,"all"), sep="-"), sep="/"), "png", sep=".")
cat("Plotting to: ", out_file, "\n")
png(out_file, width=1200, height=800)
plot(x=NULL,y=NULL,type="n", xlab="", ylab="", xlim=c(0,540),ylim=c(0,0.4))
box()
time_points <- unique(final_isr$Time)
lines(time_points, final_isr[[script_labels["line_column"]]], type="l", col="black",lwd=5)
title(main=script_labels["title_main"], xlab="Time (sec).", ylab=script_labels["title_ylab"])
dev.off()
