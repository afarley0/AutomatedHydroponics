import os
import pickle


with open(".\\data\\txt\\sequences.pickle", "rb") as handle:
	sequence_photo_dict = pickle.load(handle)
print sequence_photo_dict