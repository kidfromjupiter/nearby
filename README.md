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
This repo consists of the entirety of google's open source nearby library. Currently, it is seperated into 3 (now 2) sections. 
- Sharing
- Connections
- ~Presence~ ( *Was removed by google from their repo. RIP :(*   )

    
Linux specific implementation and compatibility shims are provided for building **Sharing** and **Connections**. 

## Why
This repo could've been a PR on the official repo. All I've done is implement the platform abstraction layer google has provided for a linux specific environment.
It's not like I haven't made a PR towards the official repo. I got tired of begging the official nearby maintainers for a PR review. So here we are. 


Moreover, all I've wanted was seamless file sharing between my android devices and my linux workstation + laptop. With this repo and the example application provided here,
it accomplishes that goal perfectly. This repo wasn't created out of any altruistic goals or out of the goodness of my heart. I had a problem. I solved it. Simple as that. 


## Documentation
Docs is on the backburner for now

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

Best place to consult would be the Github actions and workflows. 



## TODO

###  WIP

- [ ] **Transition from QT to a TUI**

    We're moving away from a GUI application to TUI application. On top of this, I've made a nearby sharing daemon with socket based IPC so that even if anyone wants to create a GUI application, it is trivial without them needing to link with nearby sharing libraries.

    This change was done to decouple the UI from the library itself since the previous implementation was difficult to work with 
      

---

### BUGS
- [ ] **Bluetooth classic bandwidth**
  
    File transfer on bluetooth classic is painfully slow. Bandwidth close to 20KB/s. ~May be a regression issue after bluetooth socket refactor~. May be an issue with sending back acknowledgements. Issue is present on pre-refactor versions. ~Look into Multiplexing maybe~ Multiplexing did not fix it : (?

- [ ] **Investigate why Bluetooth connection requests pairing.**
      
  Both the L2CAP socket and Bluetooth profile should be unauthenticated.
- [ ] **Handle existing files when receiving.**
      
  Currently, files are not overwritten if they already exist. Decide whether to overwrite, rename, or skip.

- [ ] **Bluetooth Classic transfer progress issue.**
      
  When transferring Android → Linux, Android shows 100% transferred but still says `Sending...`, while Linux lags behind. Could be a bottleneck or Android-side issue.

---

### New Features

- [ ] Upstream has been slowly adding webrtc support. Should we support it?

## Special thanks

https://github.com/proatgram and https://github.com/vibhavp

*they were the original authors of the linux platform support [PR](https://github.com/google/nearby/pull/2098) that I have based much of this codebase upon. I am standing on the shoulders
of giants*
