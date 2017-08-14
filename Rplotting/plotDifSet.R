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

# Plotting on the same graph curves for simulations done using different settings
suppressPackageStartupMessages(library("argparse"))

#commandArgs(trailingOnly = TRUE)
# create the parser object
parser <- ArgumentParser()

parser$add_argument("-d","--dir", help="absolute path of the directory containing the stored Data in .rds format", required=TRUE)
parser$add_argument("-s","--script", help="Kind of script to be sourced", default="isr")
parser$add_argument("-w","--workspace", action="store_true", help="Save workspace in .Rdata file", default=FALSE)
parser$add_argument("-y","--yaxis", type="double", default=1, help="Change the max value of the y-axis")

args <- parser$parse_args()
sources_dir = args$dir

# Simple arrays containing labels to be used that varies according to the script arg
script_labels <- vector()
if (args$script == "isr") {
  #script_labels <- c(script_labels, "Interest Satisfaction Ratio","Satisfied legitimate Interests (%)")
  script_labels <- c(script_labels, "","Satisfied legitimate Interests (%)")
} else if (args$script == "pur") {
  #script_labels <- c(script_labels, "Pit Utilization Ratio", "PIT usage (%)")
  script_labels <- c(script_labels, "", "PIT usage (%)")
}
names(script_labels) <- c("title_main","title_ylab")

files <- list.files(path=sources_dir, pattern="*.rds", full.names=T, recursive=FALSE)

plot_colors <- rainbow(length(files))
# to create the label for the legend, trim the file path as well as the file extension
# an rds filename has the format: WD-500.ipps.merged.ISR.rds
extract_name <- function(file_path){
    filen <- vector(length = 1)
    tmp <- basename(file_path)
    name_c <- unlist(strsplit(tmp,"\\."))
    filen[1] <-paste(name_c[1],name_c[2], sep=" ") 
    return(filen)
}

# if use.names is TRUE and if the function argument is a character, then sapply use the argument as names for the result unless it had names already
file_names <- sapply(files,FUN=extract_name, USE.NAMES=TRUE)

# Set the stage for plotting
out_file <- paste(paste(args$dir, "output", sep="/"), "png", sep=".")
cat("Plotting to: ", out_file, "\n")
png(out_file, width=1200, height=800)
# watch out where you put the par functions to set margins, if we move it up of one line, the png function rewrites the default values which are then used to plot
par(mar=c(5.0,5.0,2.0,1.0))
plot(x=NULL,y=NULL,type="n", xlab="", ylab="", xlim=c(0,540),ylim=c(0,args$yaxis),cex.axis=2)
box()

# add the label created above to the list of colors for the curves
names(plot_colors) <- file_names

time_points <- vector()
df_list <- vector("list",length(files))
firstTime = TRUE

# I define an array of ten symbols to be used when plotting the lines, hoping we don't need more
pch = seq(10)

i <- 1 
for(file in files){
  # I do so by creating an attribute and assigning it to the df
  # I'm renaming the columns of the loaded datasets since the script is generic, while the loaded data may be specific to different metrics, e.g., ISR
  # PUR
  tmp_df <- setNames(readRDS(file=file),c("Time","Value"))

  # I need to store also the filename, cause I need it to create the label later on
  attr(tmp_df, "filename") <- file

  df_list[[i]] <- tmp_df

  if ( firstTime ){
    time_points <- unique(tmp_df$Time)
    firstTime = FALSE
  }
  lines(time_points, tmp_df$Value, type="o", col=plot_colors[file_names[file]], lwd=4,pch=pch[i])
  i <- i + 1
}

abline(v=c(0,100,200,300,400,500),h=c(0,0.2,0.4,0.6,0.8,1),lty="dashed")
title(main=script_labels["title_main"], cex.main=2, xlab="Time (sec).", ylab=script_labels["title_ylab"],cex.lab=2)
legend("topright", names(plot_colors), pch=pch, col=plot_colors, cex=2)
dev.off()

# The following saves your workspace into a file that you can later load and inspect on a terminal
if( args$workspace ){
  save.image(file=paste(sources_dir, ".RData",sep="/"))
}
