#include "dsplib.h"
#include "mdk.h"
#include "mdkimp.h"

/*
CMDKImplementation *NewMDKImp()
{
	return new CMDKImplementation;
}
*/

void CopyStereoToMono(float *pout, float *pin, int numsamples, float amp)
{
	do
	{
		*pout++ = (pin[0] + pin[1]) * amp;
		pin += 2;
	} while(--numsamples);
}

void AddStereoToMono(float *pout, float *pin, int numsamples, float amp)
{
	do
	{
		*pout++ += (pin[0] + pin[1]) * amp;
		pin += 2;
	} while(--numsamples);
}

void CopyM2S(float *pout, float *pin, int numsamples, float amp)
{
	do
	{
		double s = *pin++ * amp;
		pout[0] = (float)s;
		pout[1] = (float)s;
		pout += 2;
	} while(--numsamples);

}

void Add(float *pout, float *pin, int numsamples, float amp)
{
	do
	{
		*pout++ += *pin++ * amp;
	} while(--numsamples);
}

void CMDKImplementation::AddInput(char const *macname, bool stereo)
{
	if (macname == NULL)
		return;

	Inputs.push_back(CInput(macname, stereo));

	SetMode();
}

void CMDKImplementation::DeleteInput(char const *macname)
{
	for (InputList::iterator i = Inputs.begin(); i != Inputs.end(); i++)
	{
		if ((*i).Name.compare(macname) == 0)
		{

			Inputs.erase(i);

			SetMode();
			return;
		}
	}
}

void CMDKImplementation::RenameInput(char const *macoldname, char const *macnewname)
{
	for (InputList::iterator i = Inputs.begin(); i != Inputs.end(); i++)
	{
		if ((*i).Name.compare(macoldname) == 0)
		{
			(*i).Name = macnewname;
			return;
		}
	}
}

void CMDKImplementation::SetInputChannels(char const *macname, bool stereo)
{
	for (InputList::iterator i = Inputs.begin(); i != Inputs.end(); i++)
	{
		if ((*i).Name.compare(macname) == 0)
		{
			(*i).Stereo = stereo;
			SetMode();
			return;
		}
	}
}

void CMDKImplementation::Input(float *psamples, int numsamples, float amp)
{
	assert(InputIterator != Inputs.end());

	if (InputIterator == Inputs.end())
	{
		pmi->pCB->MessageBox("mdk ping");
	}

	if (psamples == NULL)
	{ 
		InputIterator++;
		return;
	}


	if (numChannels == 1)
	{
		if (HaveInput == 0)
		{
			if ((*InputIterator).Stereo)
				CopyStereoToMono(Buffer, psamples, numsamples, amp);
			else
				DSP_Copy(Buffer, psamples, numsamples, amp);
		}
		else
		{
			if ((*InputIterator).Stereo)
				AddStereoToMono(Buffer, psamples, numsamples, amp);
			else
				DSP_Add(Buffer, psamples, numsamples, amp);
		}
	}
	else
	{
		if (HaveInput == 0)
		{
			if ((*InputIterator).Stereo)
				DSP_Copy(Buffer, psamples, numsamples*2, amp);
			else
				CopyM2S(Buffer, psamples, numsamples, amp);
		}
		else
		{
			if ((*InputIterator).Stereo) 
				DSP_Add(Buffer, psamples, numsamples*2, amp);
			else
				DSP_AddM2S(Buffer, psamples, numsamples, amp);
		}
	}

	HaveInput++;
	InputIterator++;

}

bool CMDKImplementation::Work(float *psamples, int numsamples, int const mode)
{
	if ((mode & WM_READ) && HaveInput)
		DSP_Copy(psamples, Buffer, numsamples);

	bool ret = pmi->MDKWork(psamples, numsamples, mode);

	InputIterator = Inputs.begin();
	HaveInput = 0;

	return ret;
}

bool CMDKImplementation::WorkMonoToStereo(float *pin, float *pout, int numsamples, int const mode)
{
	if ((mode & WM_READ) && HaveInput)
		DSP_Copy(pout, Buffer, 2*numsamples);

	bool ret = pmi->MDKWorkStereo(pout, numsamples, mode);

	InputIterator = Inputs.begin();
	HaveInput = 0;
	
	return ret;
}

	
void CMDKImplementation::Init(CMachineDataInput * const pi)
{
	ThisMachine = pmi->pCB->GetThisMachine();
	
	numChannels = 1;

	InputIterator = Inputs.begin();
	HaveInput = 0;
	MachineWantsChannels = 1;

	if (pi != NULL)
	{
		byte ver;
		pi->Read(ver);
	}
	

	pmi->MDKInit(pi);
}

void CMDKImplementation::Save(CMachineDataOutput * const po)
{
	po->Write((byte)MDK_VERSION);

	pmi->MDKSave(po);
}

void CMDKImplementation::SetOutputMode(bool stereo)
{
	numChannels = stereo ? 2 : 1;
	MachineWantsChannels = numChannels;
	
	pmi->OutputModeChanged(stereo);
}

void CMDKImplementation::SetMode()
{	
	InputIterator = Inputs.begin();
	HaveInput = 0;
	
	if (MachineWantsChannels > 1)
	{
		numChannels = MachineWantsChannels;
		pmi->pCB->SetnumOutputChannels(ThisMachine, numChannels);
		pmi->OutputModeChanged(numChannels > 1 ? true : false);
		return;
	}


	for (InputList::iterator i = Inputs.begin(); i != Inputs.end(); i++)
	{
		if ((*i).Stereo)
		{
			numChannels = 2;
			pmi->pCB->SetnumOutputChannels(ThisMachine, numChannels);
			pmi->OutputModeChanged(numChannels > 1 ? true : false);
			return;
		}
	}

	numChannels = 1;
	pmi->pCB->SetnumOutputChannels(ThisMachine, numChannels);
	pmi->OutputModeChanged(numChannels > 1 ? true : false);

}

CMDKImplementation::~CMDKImplementation()
{
}


void CMDKMachineInterfaceEx::AddInput(char const *macname, bool stereo) { pImp->AddInput(macname, stereo); }
void CMDKMachineInterfaceEx::DeleteInput(char const *macename) { pImp->DeleteInput(macename); }
void CMDKMachineInterfaceEx::RenameInput(char const *macoldname, char const *macnewname) { pImp->RenameInput(macoldname, macnewname); }
void CMDKMachineInterfaceEx::SetInputChannels(char const *macname, bool stereo) { pImp->SetInputChannels(macname, stereo); }
void CMDKMachineInterfaceEx::Input(float *psamples, int numsamples, float amp) { pImp->Input(psamples, numsamples, amp); }

bool CMDKMachineInterface::Work(float *psamples, int numsamples, int const mode) { return pImp->Work(psamples, numsamples, mode); }
bool CMDKMachineInterface::WorkMonoToStereo(float *pin, float *pout, int numsamples, int const mode) { return pImp->WorkMonoToStereo(pin, pout, numsamples, mode); }
void CMDKMachineInterface::Save(CMachineDataOutput * const po) { pImp->Save(po); }

void CMDKMachineInterface::SetOutputMode(bool stereo) { pImp->SetOutputMode(stereo); }


CMDKMachineInterface::~CMDKMachineInterface()
{
	delete pImp;
}

void CMDKMachineInterface::Init(CMachineDataInput * const pi)
{
    pImp = (CMDKImplementation*)pCB->GetNearestWaveLevel(-1,-1);
	pImp->pmi = this;

	CMDKMachineInterfaceEx *pex = GetEx();
	pex->pImp = pImp;
	pCB->SetMachineInterfaceEx(pex);

	pImp->Init(pi);
}
