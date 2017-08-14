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

# Plotting Interest Satisfaction Ratio (ISR) information traced by ndnSim L3-tracer 
# ISR is defined as the ratio of SatisfiedInterests over the TimedOutInterests+SatisfiedInterests
#
# The data contained in the trace file have the following format:
# 
# Time    Node    FaceId  FaceDescr       Type    Packets Kilobytes
# 10      server0 2       dev=local(2)    DropInterests   0       0
# 10      server0 2       dev=local(2)    OutData 0       0
# 10      server0 2       dev=local(2)    DropData        0       0
# 10      server0 0       dev[135]=net(0,135-7)   DropInterests   0       0
# 10      server0 0       dev[135]=net(0,135-7)   OutData 153     160.726
# 10      server0 0       dev[135]=net(0,135-7)   DropData        0       0
# 10      server0 -1      all     SatisfiedInterests      153     0
# 10      server0 -1      all     TimedOutInterests       0       0
suppressPackageStartupMessages(library("argparse"))

#print(commandArgs())
# create the parser object
parser <- ArgumentParser()

parser$add_argument("-p","--plot", action="store_true",default=FALSE, help="Plot the ISR over the timeline")
parser$add_argument("-f","--file", help="absolute path of the file containing the traced Data. A name without path will be only searched in the current wd", required=TRUE)
parser$add_argument("-t","--tag", default="dumbTag", help="this string is used to generate the filename of the output plot")

args <- parser$parse_args(commandArgs())
#args <- parser$parse_args()
#print(args)

filename = args$file

# preparing the stage for further plotting
# TODO: move to pdf, by now I'm plotting png because I have some problems with titles and labels on PDFs
if ( args$plot ) {
  out_file <- paste("ISR", args$tag, "png", sep=".")
  png(out_file, width=1200, height=800)
  plot(x=NULL,y=NULL,type="n", xlab="", ylab="", xlim=c(0,540),ylim=c(0,1.0))
  box()
}

# global remark, most of the following functions could be easily consolidated, I'm sorry but I'm not an R-person
# further I'd no time to polish it

pit <- read.table(filename, header=T)
pitClients <- subset(pit, grepl('client',pit$Node) & grepl("((Satisfied)|(TimedOut))Interests",pit$Type)) # Selecting only client nodes 

# you would need the two following lines, if you were interested in printing separate curves per every client
#pitClients$Node <- gsub("client","",pitClients$Node) # removing client string to make next-step numerical sorting easier
#clients <- sort(as.numeric(unique(pitClients$Node))) # vector containing all the client names in numerical order which is used later to iterate over the data frame
 
time_points <- unique(pitClients$Time) # vector of time, starting from 10 onwards moving with 10-long steps

# The following could be likely done with a call to aggregate or like-function plus a user-defined function to compute the ratio on the values of two different columns
#pit_avg <- aggregate(Packets~Time+Type, pitMRs, isr, na.rm=TRUE) # avg SatisfiedInterests and TimedOut over time, this gives you two lines per each time interval, one containing the avg satisfied and one the avg timedout
isr_func <- function(a,b){
    # compute the ratio, satisfied/(satisfied + expired)
    return (a/(a+b))
}

# because I'm lazy and I want to fix it soon, I split the operation in two pieces
satisfied <- subset(pitClients,Type=="SatisfiedInterests")
avg_satisfied <- setNames(aggregate(satisfied$Packet,list(satisfied$Time), FUN=mean),c("Time","Packets"))
timedOut <- subset(pitClients,Type=="TimedOutInterests")
avg_timedOut <- setNames(aggregate(timedOut$Packet,list(timedOut$Time), FUN=mean),c("Time","Packets"))
# now that I have averaged the values over the clients, I merge the data sets and compute the Interest Satisfaction Ratio
merged_avg <- rbind(avg_satisfied,avg_timedOut)
isr <- setNames(aggregate(merged_avg$Packets ~ merged_avg$Time, FUN = function(x){return (x[1]/(x[1] + x[2]))}),c("Time","ISR"))

if ( args$plot ) {
  # plotting isr
  lines(time_points, isr$ISR, type="l", col="black",lwd=5)
  title(main="Interest Satisfaction Ratio", xlab="Time (sec).", ylab="Satisfied legitimate Interests (%)")
  dev.off()
}
