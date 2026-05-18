# Plotting

## Minimal graphviz examples from the slides

Python example requires both a binary `graphviz` installation and the `graphviz` Python package. The latter can be installed with `pip install graphviz`.

Run with `python plot.py`

The C++ example requires a development installation of the `graphviz` library. The CMakeLists.txt file is set up to find the library in the default installation location. If you have installed it in a different location, you can set the `Graphviz_DIR` variable to point to the installation directory.
Run with `cmake . && make && ./plot`

## Dependency installation

For the Python example, you can just use `conda` to install the required dependencies.

```bash
conda install graphviz python-graphviz
```
The python part is also available via `pip`:

```bash
pip install graphviz
```

For the C++ example, you'll need the library and headers for `graphviz`. On Ubuntu, you can install them with:

```bash
sudo apt-get install graphviz libgraphviz-dev
```
On MacOS, you can install them with `brew`:

```bash
brew install graphviz
```

On Windows you might need to install the library manually. You can find the installation instructions on the [graphviz website](https://graphviz.gitlab.io/download/). You can download the source code.

```bash
git clone –recurse-submodules https://gitlab.com/graphviz/graphviz.git
```
