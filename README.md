# Perfect Toast Machine

![Shawn holding toast from the perfect toast machine](images/ptm-and-shawn.jpg)

All the code needed to build the perfect toast machine, an AI-powered device that makes the perfect toast using odor.

A full walkthrough for building the device can be found here: [How to Build an AI-powered Toaster](https://www.digikey.com/en/maker/projects/how-to-build-an-ai-powered-toaster/2268be5548e74ceca6830bf35f0f0f9e)

The Edge Impulse project used to train the machine learning model can be found here: [perfect-toast-machine](https://studio.edgeimpulse.com/public/129477/latest). You'll want to clone the project, go to *Deployment*, download the Arduino .zip library, and install it as a library in the Arduino IDE in order to run the *perfect-toast-machine* Arduino code.

## Contents

 * *datasets/* - Data that I captured building the project. You are welcome to use it, but I can't promise it will work in your environment.
 * *images/* - Images used for this README.
 * *perfect-toast-machine/* - Arduino code used for deployment. You'll need to install the .zip Arduino library from the Edge Impulse project first.
 * *tests/* - Internal tests that I used while making this project. You should not need to use them (unless you want to see how I test embedded machine learning code).
 * *toast-odor-data-collection/* - Run this code on your Arduino to collect raw data from the gas sensors. Used in tandem with *serial-data-collect-csv.py* to log raw data to CSV files to your computer.
 * *ptm-dataset-curation.ipynb* - Jupyter Notebook script (meant to be run on Google Colab) used to curate and standardize the dataset. Run this first after collecting your dataset before you upload it to Edge Impulse for training.
 * *README.md* - You're looking at it.
 * *serial-data-collect-csv.py* - Python (v3+) script used to collect raw data from the *toast-odor-data-collection* Arduino program and log to CSV files. See [this repo](https://github.com/edgeimpulse/example-data-collection-csv) for an example on how to use the script.

## License

All code, unless otherwise noted, is licensed under the [Zero-Clause BSD (0BSD) license](https://opensource.org/licenses/0BSD).

Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.