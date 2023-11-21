#%%

from PIL import Image
from PIL.ExifTags import TAGS
import datetime
import os
import subprocess
import time
import shutil

#%%

# for earlier versions of remote
source_path = "/storage/external_sd/DJI/dji.go.v4/DJI Album"
target_path_temp = "C:\\Users\\elliot\\Downloads\\DJI\\temp"
target_path_output = "C:\\Users\\elliot\\Downloads\\DJI\\output"

#%%
def get_exif_datetime(file_path):
	try:
		img = Image.open(file_path)
		exif_data = img._getexif()
		for tag, value in exif_data.items():
			decoded = TAGS.get(tag, tag)
			if decoded == "DateTimeOriginal":
				return datetime.datetime.strptime(value, '%Y:%m:%d %H:%M:%S')
	except Exception as e:
		print(f"Error reading EXIF data: {e}")
	return None

def set_file_modification_time(file_path, mod_time):
	try:
		os.utime(file_path, (mod_time.timestamp(), mod_time.timestamp()))
	except Exception as e:
		print(f"Error setting file modification time: {e}")

def rename_file(file_path, mod_time):
	try:
		directory, filename = os.path.split(file_path)
		new_filename = mod_time.strftime("%Y%m%d_%H%M%S.jpg")
		new_file_path = os.path.join(directory, new_filename)
		os.rename(file_path, new_file_path)
	except Exception as e:
		print(f"Error renaming file: {e}")


def wait_and_move_file(src, dst, timeout=10):
	start_time = time.time()
	while True:
		try:
			shutil.move(src, dst)
			break  # If the move is successful, exit the loop
		except PermissionError:
			time_elapsed = time.time() - start_time
			if time_elapsed > timeout:
				print(f"Timeout reached while waiting to move file: {src}")
				break  # Exit the loop if the timeout is reached
			time.sleep(1)  # Wait for a second before retrying

def move_file_to_output(file_path, mod_time):
	try:
		directory, filename = os.path.split(file_path)
		new_filename = mod_time.strftime("%Y%m%d_%H%M%S.jpg")
		new_file_path = os.path.join(target_path_output, new_filename)
		wait_and_move_file(file_path, new_file_path)
	except Exception as e:
		print(f"Error renaming file: {e}")

def get_file_list():
	try:
		# Replace with your directory path
		escaped_source_path = source_path.replace(" ", "\\ ")
		result = subprocess.check_output(["adb", "shell", "ls", escaped_source_path])
		files = result.decode().splitlines()
		return set(files)
	except subprocess.CalledProcessError as e:
		raise(Exception("Error listing files:", e))

def main():
	try:
		last_files = get_file_list()
	except:
		return

	while True:
		current_files = get_file_list()

		new_files = current_files - last_files

		time.sleep(1)  # Wait for 1 second (we wait here to give time for file to become complete)

		if new_files:
			print("New files detected:", new_files)
			for file in new_files:
				# Replace with your local destination folder
				local_path = os.path.join(target_path_temp, file)
				remote_path = f'"{source_path}/{file}"'
				
				# Prepare the command
				command = ["adb", "pull", remote_path, local_path]

				# Print the command for debugging
				# print("Executing command:", ' '.join(command))

				try:
					subprocess.run(' '.join(command))
					print(f"File {file} pulled successfully.")
				except subprocess.CalledProcessError as e:
					print(f"Error pulling file {file}:", e)

				# New code to process file
				mod_time = get_exif_datetime(local_path)
				if mod_time:
					set_file_modification_time(local_path, mod_time)
					move_file_to_output(local_path, mod_time)

		last_files = current_files

if __name__ == "__main__":
	main()

# %%
