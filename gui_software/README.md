# Python GUI Application

## Dependencies

1. To run the Python GUI code, you will need jupyter notebooks, which you can access by installing [Anaconda](https://docs.anaconda.com/free/anaconda/install/windows/) and [Anaconda Navigator](https://docs.anaconda.com/free/navigator/install/).

2. Once they are installed, open up Anaconda Navigator, click on "Environments" and create a new environment with the Python package installed (the default Python 3.10.14 version is fine).

3. Open up Anaconda prompt and run: 
```
conda activate [name of your environment]
```

4. Download the required packages by running:
```
conda install -n test jupyter
```

```
pip install customtkinter
```

```
pip install tkintermapview
```

```
pip install thingspeak
```

```
conda install numpy
```

5. Open up jupyter notebooks in your environment by running:

```
jupyter notebook
```

6. Navigate to the "LoRaWAN_GUI.ipynb" file and open it. The code is ready to be run. To ensure that there are no errors, make sure that you select "Kernel->Restart and Run All Cells" everytime that you want to reopen the GUI.

## How It Works