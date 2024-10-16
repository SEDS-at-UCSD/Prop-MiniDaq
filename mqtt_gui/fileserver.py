from fastapi import FastAPI, UploadFile, File, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse
import os
import shutil
import requests

app = FastAPI()

# Add CORS middleware
app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:3000", "http://localhost:8001"],  # React app origin
    allow_credentials=True,
    allow_methods=["*"],  # Allow all methods (GET, POST, etc.)
    allow_headers=["*"],  # Allow all headers
)

# Files will be in the same folder as fileserver.py
allowed_files = [
    'conversion_factor_config.json',
    'abort_config.yaml',
    'automation_config.yaml'
]

# Endpoint to retrieve file contents
@app.get("/files/{filename}")
async def read_file(filename: str):
    if filename not in allowed_files:
        raise HTTPException(status_code=404, detail="File not found")

    filepath = os.path.join(os.getcwd(), filename)
    if os.path.exists(filepath):
        return FileResponse(filepath)
    raise HTTPException(status_code=404, detail="File not found")

# Notify main.py to reload configs
def notify_main_update(filename):
    try:
        # Assuming main.py is running on localhost and listening on port 8001
        response = requests.post(f"http://localhost:8001/update_config/{filename}")
        if response.status_code == 200:
            print(f"Main notified of {filename} update successfully")
        else:
            print(f"Failed to notify main: {response.status_code}, {response.text}")
    except Exception as e:
        print(f"Error notifying main: {str(e)}")

# Endpoint to upload/replace a file
@app.post("/upload/{filename}")
async def upload_file(filename: str, file: UploadFile = File(...)):
    if filename not in allowed_files:
        raise HTTPException(status_code=400, detail="Invalid file name")

    filepath = os.path.join(os.getcwd(), filename)
    try:
        with open(filepath, "wb") as buffer:
            shutil.copyfileobj(file.file, buffer)
        notify_main_update(filename)
        return {"message": f"{filename} uploaded successfully"}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"File upload failed: {str(e)}")

# Endpoint to list all allowed files
@app.get("/files")
async def list_files():
    return {"allowed_files": allowed_files}

if __name__ == '__main__':
    import uvicorn
    uvicorn.run(app, host="localhost", port=8000) #localhost
