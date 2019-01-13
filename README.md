# NDN-IFA
This repository contains software and documentation to mount some specific Interest Flooding Attacks (IFAs) and measure their impact in Named-Data Networks. Those IFAs adhere to an attack model proposed for the first time in [1]. The attacks have been implemented and tested on the simulation environment ndnSIM. The code has only been tested on the version 1.0 of the ndnSim simulator. Therefore, some modules may not work and/or require modifications when applied to later versions of ndnSim.
The repository contains three main folders. Each folder includes one more README file dissecting the content of the directory. The 'ndnSim1.0-code' folder contains software modules (applications, forwading strategies, tracers and other utilities) to launch this kind of IFAs in simulated network environments.


NAMES
-----------------------------------------------
This repository contains custom ndnSim applications reading content identifiers from text files. This was meant to have realistic names in simulations as part of a steadier attack model for IFAs. A publicly available list of Wikipedia page titles has been leveraged to simulate a large number of different content identifiers. Up-to-date lists of the wikipedia page titles are made available by the Wikimedia foundation. Experiments in [1] used the list of page titles for the English version of the Wikipedia encyclopedia dated 2016-03-28. The original archive has not been included in this repository because of its large size. Follows some information about that archive:

archive name: enwiki-20160328-all-titles-in-ns-0.gz
Once available at: https://dumps.wikimedia.org/other/pagetitles/20160328/enwiki-20160328-all-titles-in-ns-0.gz (this link is not valid anymore, the wikimedia foundation may have way and format to disseminate this kind of files)
md5sum: 079e8d33138109bfee633544820dd711

That list contains 124127128 different page titles.
The process of generation of the lists of fake names used for the experiments in [1] has been described therein. Those lists have been included in the folder "names". 

IMPORTANT NOTES
-----------------------------------------------

If you use some of this code for your research, you may also consider citing the following work which has led to the development of most of the software in this repository:

Signorello S, Marchal S, François J, Festor O, State R. Advanced interest flooding attacks in named-data networking. In2017 IEEE 16th International Symposium on Network Computing and Applications (NCA) 2017 Oct 30 (pp. 1-10). IEEE.
