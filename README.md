# Google Nearby for Linux
> ## 🚧 **Under Construction** 🚧  
> This repo is actively being worked on. Things are buggy, builds may fail, documentation is non-existent, and we test in prod.
>
> **You have been warned**

---

<img width="600" height="464" alt="image" src="https://github.com/user-attachments/assets/9533ea09-81d9-4162-b90f-e0bd8b714d1d" />



## Demo


https://github.com/user-attachments/assets/048afa1e-40a4-4351-a859-c81b642fc6e3

## What
This repo consists of the entirety of google's open source nearby library. Currently, it is seperated into 3 sections. 
- Sharing
- Connections
- ~Presence~ ( Build fails for some reason. Haven't had time to investigate. Maybe something minor. Check the validate.yaml for build steps and see why its faiing )

    
Linux specific implementation and compatibility shims are provided for building **Sharing** and **Connections**. 
Nearby presence may or may not build.

## Why
This repo could've been a PR on the official repo. All I've done is implement the platform abstraction layer google has provided for a linux specific environment.
It's not like I haven't made a PR towards the official repo. I got tired of begging the official nearby maintainers for a PR review. So here we are. 


Moreover, all I've wanted was seamless file sharing between my android devices and my linux workstation + laptop. With this repo and the example application provided here,
it accomplishes that goal perfectly. This repo wasn't created out of any altruistic goals or out of the goodness of my heart. I had a problem. I solved it. Simple as that. 


## How
### How to use
Proper documentation is coming I swear. University's getting pretty hectic so docs is on the backburner. The wiki has some more information but it's nowhere near a proper documentation. It has some good brief
overviews and where generally everything is located if you're thinking about contributing. 

If you want any clarification on anything, feel free to open an issue. I'll get back to you ASAP.

As a consolation prize, I've indexed this project using [Deepwiki](https://deepwiki.com/kidfromjupiter/nearby). You might have strong feeling about AI use. But I feel like documenting very large codebases is a perfect usecase for such models. (They are called Large Language Models for a reason )

### How to install
This repo provides prebuilt binaries of the Quick Share application. The only officially supported distro is Fedora 43 for now. The newest ubuntu images *should* work fine
although that needs to be tested. I want to support more distros so if you encounter issues installing on your distro, please let me know. 

>**NOTE: Previosly, the quickshare binary required the `sdbus-cpp` v2 library installed on the system. This is no longer the case and it is bundled with the shared library. Hopefully this expands compatibility**

#### Prerequisites

- `systemd`
- `NetworkManager`
- `bluez >= 5.85`

**To install the prerequisites, run this command**

```bash
sudo dnf install -y \
  bluez bluez-libs bluez-libs-devel \
  sdbus-cpp sdbus-cpp-devel
```
---

**To install the Quick Share application,**

1. Go to [releases](https://github.com/kidfromjupiter/nearby/releases)
2. Download the latest `nearby-file-share-linux-*.tar.gz`
3. `mkdir -p nearby && tar -xf nearby-file-share-linux-*.tar.gz -C nearby`
4. `cd nearby`
5. `chmod +x install_nearby_file_share.sh`
6. `./install_nearby_file_share.sh`

**To install the actual library and headers,**

Currently there are no prebuilt shared library or headers. You'll have to build them yourself

### How to build
~Check the [wiki](https://github.com/kidfromjupiter/nearby/wiki/Development-Environment-and-Building)~

Wiki isn't built yet. Best place to consult would be the Github actions and workflows. 
### How to contribute
~Check the [wiki](https://github.com/kidfromjupiter/nearby/wiki/Development-Environment-and-Building)~


## TODO

> **Development is paused until my next break (around mid-June.)**

---

### 🔴 P0 — Critical / Core Functionality

> Issues that break core functionality. These should be addressed immediately.

- **Merge latest upstream changes**
  

- **Bluetooth classic bandwidth**
  
    File transfer on bluetooth classic is painfully slow. Bandwidth close to 20KB/s. ~May be a regression issue after bluetooth socket refactor~. May be an issue with sending back acknowledgements. Issue is present on pre-refactor versions. ~Look into Multiplexing maybe~ Multiplexing did not fix it : (?
- **~Linux → Android~ file sharing is unreliable** after the newest Android Quick Share updates.  
  Investigate why and fix it. ~Possibly related to proprietary certificate changes~. Should probably add unit tests and integration tests for each medium. Everything is so fucking buggy it makes me wanna rip my fucking eyes out. Might be related to recent upstream changes. I could probably properly test linux -> linux bidirectional sharing. Will need to simulate a lot of hardware stuff though. Since there's no stable reference platform to write automatic tests against writing linux to linux tests could be like clown to clown communication
  
   <img width="200" height="143" alt="image" src="https://github.com/user-attachments/assets/caed18f3-1337-4499-ba4d-b49f549c0cf5" />



- **QR code scanning does not work.**  
  Likely related to the Linux → Android sharing issue above. I did get it working once in a very old build. So it shouldn't impossible. Unless google changed something 

---

### 🟠 P1 — Important Annoyances

> Problems that are not fully blocking, but noticeably affect usability.

- **Investigate why Bluetooth connection requests pairing.**  
  Both the L2CAP socket and Bluetooth profile should be unauthenticated.
- **Handle existing files when receiving.**  
  Currently, files are not overwritten if they already exist. Decide whether to overwrite, rename, or skip.

---

### 🟡 P2 — Quality of Life / Cleanup

> Improvements that would make the project cleaner, smoother, or easier to maintain.

- **Bluetooth Classic transfer progress issue.**  
  When transferring Android → Linux, Android shows 100% transferred but still says `Sending...`, while Linux lags behind. Could be a bottleneck or Android-side issue.
- **Add tests for basically everything.**
- **Clean up `implementation/linux`.**  
  Linux-specific implementation files are currently all in one directory. This matches the other platforms, but creates visual bloat.
- **Document basically everything.**  
  This would be a large project on its own.
- **Resolve random crashes in the Quick Share application.**

---

### 🔵 P3 — New Features

> Non-essential features and future improvements.

- **Support fast initiation.**
- Upstream has been slowly adding webrtc support. Should we support it?


## Apologies
I may have done things in *incredibly* stupid and overcomplicated ways. It doesn't certainly help that this was the way I decided to learn C++. Blessed be my naive soul. I also do not have much experience working with such 
enormous codebases. 

If you see any such stupidities, feel free to berate me in the most shameless of manners in an issue. I look forward to learning how to do it the proper way and to improve my atrocious code quality.

## Special thanks

https://github.com/proatgram and https://github.com/vibhavp

*they were the original authors of the linux platform support [PR](https://github.com/google/nearby/pull/2098) that I have based much of this codebase upon. I am standing on the shoulders
of giants*
