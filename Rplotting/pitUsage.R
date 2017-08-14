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

# Plotting PIT usage information traced by ndnSim

# Loading the data file into a data-frame keeping the original header; the file follows the format:
#
#  Time     Node                  Signal     Value
#  1   10       56                PitUsage    0.0000
#  2   10       56              PitEntries    0.0000
#  3   10 monitor6                PitUsage    0.0284
#  4   10 monitor6              PitEntries  142.0000
#  5   10 monitor6  EntriesSatisfiedBefore 1382.0000
#  6   10 monitor6 MaliciousRequestedMulti 1459.0000 
#
# All the nodes writes PIT info every 10 seconds. Normal nodes (e.g., node 56 above) report PitUsage and PitEntries. MRs (e.g., monitor6 above) also report EntriesSatisfiedBefore and MaliciousRequestedMulti.
# For sake of plotting the PIT consumption, we are only interested in the PitUsage Value.

suppressPackageStartupMessages(library("argparse"))

parser <- ArgumentParser()

parser$add_argument("-p","--plot", action="store_true",default=FALSE, help="Plot the ISR over the timeline")
parser$add_argument("-f","--file", help="absolute path of the file containing the traced Data. A name without path will be only searched in the current wd", required=TRUE)
parser$add_argument("-s","--pitSize", type="integer", default=5000, help="Pit Size used to avg PitEntries values")
parser$add_argument("-e","--entries", action="store_true", help="When specified the PitEntries values are considered instead of the PitUsage ones", default=FALSE)
parser$add_argument("-n","--node", help="Kind of node for which the data must be plot, by default all nodes are considered", default="all")
parser$add_argument("-t","--tag", default="dumbTag", help="this string is used to generate the filename of the output plot")

# Follow two different lines to fecth the arguments for the script, every time one is commented out and the other is not. The reason is that the 2nd line works if you want to run the script individually on a single file, while the 1st line works when you call the script from within other scripts, like mergePlots.R
#args <- parser$parse_args(commandArgs())
args <- parser$parse_args()

pit <- read.table(args$file, header=T)

rowType <- "PitUsage"
if ( args$entries ) {
  rowType <- "PitEntries"
}

# Extract a list of nodes that never report any value for their PITs. Those nodes are not involved in any sort of forwarding
# I bet this could be done in a smarter way, but I've not time to figure it out, so I do it as follows:
# 1 I sum all the PitUsage values for every node
# 2 I select the node IDs whose sums equal to zero
summedPITs <- aggregate(Value ~ Node + Signal, data=pit, FUN=sum)
emptyNodes <- unique(summedPITs$Node[summedPITs$Signal==rowType & summedPITs$Value == 0.0000]) 
#'%ni%' <- function(x,y)!('%in%'(x,y))
'%ni%' <- Negate('%in%') # quick function definition to have an inverse of the function toBeInTheList
# 3 I remove the nodes for which no pit values are reported. 
cleanedPit <- pit[pit$Node %ni% emptyNodes,]

all_nodes = FALSE
switch(args$node,
		 all={
 		   pitMRs <- subset(cleanedPit, grepl(rowType,cleanedPit$Signal)) # Selecting the rows which contain the PIT usage for all the nodes 
		   all_nodes = TRUE
		 },
		 {
		  pitMRs <- subset(cleanedPit, grepl('monitor',cleanedPit$Node) & grepl(rowType,cleanedPit$Signal)) # Selecting only the rows which contain the PIT usage for the monitor nodes 
		 } 
		)

MRs <- sort(unique(pitMRs$Node)) # vector containing all the MR names in numerical order which is used later to iterate over the data frame
 
time_points <- unique(pitMRs$Time) # vector of time, range from 10...540 with 10-long steps 

# If PitEntries is selected, perform the following steps
# sum all the values with the same time
# count them, how many lines
# multiply the total * the pitSize and store it in xValue
# divide the sum by xValue
if ( args$entries ) {
  summedEntries <- aggregate(Value ~ Time, data=pitMRs, FUN=sum)

  xValue <- length(MRs) * args$pitSize

  tmp <- cbind(summedEntries, mapply(function(x,y) x/y, summedEntries$Value, xValue))

  pit_avg <- setNames(subset(tmp,select=c(1,3)), c("Time","PUR"))
}


# TODO: move to pdf, by now I'm plotting png because I have some problems with titles and labels on PDFs
plot_colors <- rainbow(length(MRs))
names(plot_colors) <- MRs
if ( args$plot ) {
  outpath <- dirname(normalizePath(args$file))
  filename <- paste("PitUsage", args$tag, "png", sep=".")
  fullPath <- paste(outpath, filename, sep="/")
  png(fullPath, width=1200, height=800)
  plot(x=NULL,y=NULL,type="n", xlab="", ylab="", xlim=c(0,540),ylim=c(0,0.6))
  box()
}

# the following loops over the data frame using monitors names to select the appropriate rows
# then it draws a line plot for the pit usage of the selected monitor node
for(mr in MRs){
  # it is important to define the pattern like follows, otherwise if you just use the mr value, you'll take multiple nodes' values. For ex., the node 0 would take
  # the values from rows containing both '0' and 'monitor0' and the script would crash because of different sizes of x and y vectors for the subsequent line command
  pattern = paste("^", mr, "$", sep="")
  tmp_mr <- subset(pitMRs, grepl(pattern, pitMRs$Node))
  if ( args$plot & !all_nodes) {
     lines(time_points, tmp_mr$Value, type="l", col=plot_colors[mr])
  }

}

# Finally we plot the avg line chart for all the MRs
#pit_avg <- setNames(aggregate(pitMRs$Value,list(pitMRs$Time), FUN=mean), c("Time","PUR") )# aggregate by computing a mean of the 1st parameter values for all the rows with the same 2nd parameter value
if ( args$plot ) {
  lines(time_points, pit_avg$PUR, type="l", col="black",lwd=3)
  
  title(main="PIT usage of the monitoring nodes",xlab="Time (sec).", ylab="PIT usage (%)")
  
  # Add the black line for the avg and plot a legend on the chart
  # TODO: I must fix the legend for the 'all' case, where I only need to visualize only one line, i.e., the avg, but I'm still plotting all the nodes colors and numbers
  plot_colors <- append(plot_colors,"black")
  names(plot_colors)[length(plot_colors)] <- "avg"
  legend("topright", names(plot_colors), pch=15, col=plot_colors)
  dev.off()
}
