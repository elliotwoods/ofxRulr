#%%

import subprocess
import time
import os

#%%
source_path = "/storage/external_sd/DJI/dji.go.v4/DJI Album"
target_path = "C:\\Users\\elliot\\Downloads\\DJI"

#%%
def get_file_list():
	try:
		# Replace with your directory path
		escaped_source_path = source_path.replace(" ", "\\ ")
		result = subprocess.check_output(["adb", "shell", "ls", escaped_source_path])
		files = result.decode().splitlines()
		return set(files)
	except subprocess.CalledProcessError as e:
		raise("Error listing files:", e)
		

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
				local_path = os.path.join(target_path, file)
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


		last_files = current_files

if __name__ == "__main__":
	main()

# %%
